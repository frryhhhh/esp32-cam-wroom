#include "hw_init.h"
#include "config.h"

#include <LittleFS.h>
#include <driver/gpio.h>
#include <driver/i2s_std.h>

#if defined(ENABLE_CAMERA)
#include "esp_camera.h"
#endif

volatile bool g_cam_ready = false;
volatile uint16_t g_cam_last_err = 0;
volatile bool g_mic_ready = false;
volatile bool g_fs_ready = false;

static volatile ButtonId s_virtual_btn = BTN_NONE;
static ButtonId s_last_raw_btn = BTN_NONE;
static uint32_t s_last_debounce_ms = 0;
static constexpr uint32_t DEBOUNCE_MS = 45;

#if defined(ENABLE_MIC_I2S)
static i2s_chan_handle_t s_rx_chan = nullptr;
#endif

void hw_init_early() {
  // Виртуальный GND на GPIO 3 для внешних модулей, где физические GND/3V3 заняты
  pinMode(PIN_VIRTUAL_GND, OUTPUT);
  digitalWrite(PIN_VIRTUAL_GND, LOW);
  gpio_set_drive_capability((gpio_num_t)PIN_VIRTUAL_GND, GPIO_DRIVE_CAP_3);

  Serial.printf("[HW] Virtual GND initialized on GPIO %d = LOW\n", PIN_VIRTUAL_GND);
  Serial.printf("[HW] Free Heap: %u, Free PSRAM: %u\n", ESP.getFreeHeap(), ESP.getFreePsram());
}

void hw_init_buttons() {
  const int pins[] = {
    PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_LEFT,
    PIN_BTN_RIGHT, PIN_BTN_OK, PIN_BTN_BACK
  };
  for (int p : pins) {
    pinMode(p, INPUT_PULLUP);
  }
  Serial.println("[HW] Buttons initialized (INPUT_PULLUP -> GND)");
}

static ButtonId read_raw_physical_button() {
  if (digitalRead(PIN_BTN_UP) == LOW)    return BTN_UP;
  if (digitalRead(PIN_BTN_DOWN) == LOW)  return BTN_DOWN;
  if (digitalRead(PIN_BTN_LEFT) == LOW)  return BTN_LEFT;
  if (digitalRead(PIN_BTN_RIGHT) == LOW) return BTN_RIGHT;
  if (digitalRead(PIN_BTN_OK) == LOW)    return BTN_OK;
  if (digitalRead(PIN_BTN_BACK) == LOW)  return BTN_BACK;
  return BTN_NONE;
}

void hw_inject_virtual_button(ButtonId btn) {
  if (btn != BTN_NONE) {
    s_virtual_btn = btn;
  }
}

ButtonId hw_poll_button() {
  if (s_virtual_btn != BTN_NONE) {
    ButtonId v = s_virtual_btn;
    s_virtual_btn = BTN_NONE;
    return v;
  }

  ButtonId raw = read_raw_physical_button();
  uint32_t now = millis();

  if (raw != s_last_raw_btn) {
    s_last_raw_btn = raw;
    s_last_debounce_ms = now;
    return BTN_NONE;
  }

  static ButtonId s_reported_btn = BTN_NONE;
  if ((now - s_last_debounce_ms) >= DEBOUNCE_MS) {
    if (raw != BTN_NONE && s_reported_btn != raw) {
      s_reported_btn = raw;
      return raw;
    }
    if (raw == BTN_NONE) {
      s_reported_btn = BTN_NONE;
    }
  }
  return BTN_NONE;
}

void hw_init_fs() {
  if (LittleFS.begin(true)) {
    g_fs_ready = true;
    Serial.printf("[FS] LittleFS mounted. Total: %u, Used: %u\n",
                  LittleFS.totalBytes(), LittleFS.usedBytes());
  } else {
    g_fs_ready = false;
    Serial.println("[FS] ERROR: LittleFS mount failed!");
  }
}

void hw_init_camera_safe() {
  g_cam_ready = false;
  g_cam_last_err = 0;

#if !defined(ENABLE_CAMERA)
  Serial.println("[CAM] ENABLE_CAMERA is disabled in config.h. Skipping camera init.");
  return;
#else
  if (!psramFound()) {
    g_cam_last_err = 0x105;
    Serial.println("[CAM] ERROR 0x105: PSRAM not found, skipping OV2640 init safely.");
    return;
  }

  // Небольшая пауза для стабилизации питания и линий после включения виртуального GND
  delay(120);

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_1;
  config.ledc_timer   = LEDC_TIMER_1;
  config.pin_d0       = PIN_CAM_Y2;
  config.pin_d1       = PIN_CAM_Y3;
  config.pin_d2       = PIN_CAM_Y4;
  config.pin_d3       = PIN_CAM_Y5;
  config.pin_d4       = PIN_CAM_Y6;
  config.pin_d5       = PIN_CAM_Y7;
  config.pin_d6       = PIN_CAM_Y8;
  config.pin_d7       = PIN_CAM_Y9;
  config.pin_xclk     = PIN_CAM_XCLK;
  config.pin_pclk     = PIN_CAM_PCLK;
  config.pin_vsync    = PIN_CAM_VSYNC;
  config.pin_href     = PIN_CAM_HREF;
  config.pin_sccb_sda = PIN_CAM_SIOD;
  config.pin_sccb_scl = PIN_CAM_SIOC;
  config.pin_pwdn     = PIN_CAM_PWDN;
  config.pin_reset    = PIN_CAM_RESET;

  // Сниженная частота XCLK повышает устойчивость при возможных наводках/флюсе на FPC
  config.xclk_freq_hz = 12000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA; // 320x240 — оптимально для стабильного MJPEG без OVF
  config.jpeg_quality = 14;
  config.fb_count     = 1;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    g_cam_last_err = (uint16_t)(err & 0xFFFF);
    if (g_cam_last_err == 0) {
      g_cam_last_err = 0x105;
    }
    Serial.printf("[CAM] WARNING: esp_camera_init failed with err=0x%X (code 0x%04X). Continuing without camera!\n",
                  (unsigned)err, g_cam_last_err);
    esp_camera_deinit();
    g_cam_ready = false;
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s == nullptr) {
    g_cam_last_err = 0x105;
    Serial.println("[CAM] WARNING: sensor_get returned NULL (0x105). Deinitializing camera safely.");
    esp_camera_deinit();
    g_cam_ready = false;
    return;
  }

  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_gain_ctrl(s, 1);
  s->set_exposure_ctrl(s, 1);

  g_cam_ready = true;
  g_cam_last_err = 0;
  Serial.println("[CAM] OV2640 initialized successfully.");
#endif
}

bool hw_capture_jpeg(uint8_t** out_buf, size_t* out_len) {
  *out_buf = nullptr;
  *out_len = 0;

#if !defined(ENABLE_CAMERA)
  return false;
#else
  if (!g_cam_ready) {
    return false;
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || !fb->buf || fb->len == 0) {
    if (fb) {
      esp_camera_fb_return(fb);
    }
    Serial.println("[CAM] WARNING: Frame capture timeout/error -> marking camera unavailable safely.");
    g_cam_last_err = 0x105;
    g_cam_ready = false;
    return false;
  }

  uint8_t* copy = (uint8_t*)ps_malloc(fb->len);
  if (!copy) {
    copy = (uint8_t*)malloc(fb->len);
  }
  if (!copy) {
    esp_camera_fb_return(fb);
    return false;
  }

  memcpy(copy, fb->buf, fb->len);
  *out_buf = copy;
  *out_len = fb->len;
  esp_camera_fb_return(fb);
  return true;
#endif
}

void hw_release_jpeg(uint8_t* buf) {
  if (buf) {
    free(buf);
  }
}

void hw_init_mic_safe() {
  g_mic_ready = false;

#if !defined(ENABLE_MIC_I2S)
  Serial.println("[MIC] ENABLE_MIC_I2S disabled.");
  return;
#else
  // Жёсткая проверка: пины 4, 5, 6 запрещены для I2S из-за конфликта с камерой
  if (PIN_I2S_WS == 4 || PIN_I2S_WS == 5 || PIN_I2S_WS == 6 ||
      PIN_I2S_SCK == 4 || PIN_I2S_SCK == 5 || PIN_I2S_SCK == 6 ||
      PIN_I2S_SD == 4 || PIN_I2S_SD == 5 || PIN_I2S_SD == 6) {
    Serial.println("[MIC] CRITICAL: Forbidden I2S pin (4, 5, 6) detected! Aborting I2S init.");
    return;
  }

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.auto_clear = true;

  esp_err_t err = i2s_new_channel(&chan_cfg, nullptr, &s_rx_chan);
  if (err != ESP_OK || s_rx_chan == nullptr) {
    Serial.printf("[MIC] i2s_new_channel failed: 0x%X (mic not connected yet)\n", (unsigned)err);
    return;
  }

  i2s_std_config_t std_cfg = {};
  std_cfg.clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE);
  std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
  std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  std_cfg.gpio_cfg.bclk = (gpio_num_t)PIN_I2S_SCK;
  std_cfg.gpio_cfg.ws   = (gpio_num_t)PIN_I2S_WS;
  std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
  std_cfg.gpio_cfg.din  = (gpio_num_t)PIN_I2S_SD;
  std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
  std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
  std_cfg.gpio_cfg.invert_flags.ws_inv   = false;

  err = i2s_channel_init_std_mode(s_rx_chan, &std_cfg);
  if (err != ESP_OK) {
    Serial.printf("[MIC] i2s_channel_init_std_mode failed: 0x%X\n", (unsigned)err);
    i2s_del_channel(s_rx_chan);
    s_rx_chan = nullptr;
    return;
  }

  err = i2s_channel_enable(s_rx_chan);
  if (err != ESP_OK) {
    Serial.printf("[MIC] i2s_channel_enable failed: 0x%X\n", (unsigned)err);
    i2s_del_channel(s_rx_chan);
    s_rx_chan = nullptr;
    return;
  }

  // Проверяем наличие реального сигнала/активного потока с неподключённого INMP441
  int16_t probe[64] = {0};
  size_t bytes_read = 0;
  esp_err_t r_err = i2s_channel_read(s_rx_chan, probe, sizeof(probe), &bytes_read, 120);
  int32_t energy = 0;
  size_t samples = bytes_read / sizeof(int16_t);
  for (size_t i = 0; i < samples; i++) {
    int32_t v = probe[i];
    energy += (v >= 0) ? v : -v;
  }

  if (r_err == ESP_OK && samples > 0 && energy > 0) {
    g_mic_ready = true;
    Serial.println("[MIC] INMP441 I2S active and producing data.");
  } else {
    g_mic_ready = false;
    Serial.println("[MIC] INMP441 not detected / silent stream -> /audio will return 503 until connected.");
  }
#endif
}

size_t hw_read_mic_pcm16(int16_t* dst_samples, size_t max_samples) {
#if !defined(ENABLE_MIC_I2S)
  (void)dst_samples;
  (void)max_samples;
  return 0;
#else
  if (!g_mic_ready || s_rx_chan == nullptr || dst_samples == nullptr || max_samples == 0) {
    return 0;
  }

  size_t bytes_read = 0;
  esp_err_t err = i2s_channel_read(s_rx_chan, dst_samples, max_samples * sizeof(int16_t), &bytes_read, 150);
  if (err != ESP_OK || bytes_read == 0) {
    g_mic_ready = false;
    return 0;
  }

  size_t count = bytes_read / sizeof(int16_t);
  int32_t sum_abs = 0;
  for (size_t i = 0; i < count; i++) {
    int32_t v = dst_samples[i];
    sum_abs += (v >= 0) ? v : -v;
  }
  if (sum_abs == 0) {
    g_mic_ready = false;
    return 0;
  }
  return count;
#endif
}
