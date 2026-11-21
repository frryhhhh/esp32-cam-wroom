#include "display_lvgl.h"

static LGFX_ST7789 s_lcd;
static uint32_t s_last_tick_ms = 0;

static constexpr size_t DRAW_BUF_LINES = 24;
static constexpr size_t DRAW_BUF_BYTES = LCD_WIDTH * DRAW_BUF_LINES * sizeof(lv_color_t);

static void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  if (s_lcd.getStartCount() == 0) {
    s_lcd.startWrite();
  }
  uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
  s_lcd.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t*)px_map);
  s_lcd.endWrite();
  lv_display_flush_ready(disp);
}

void display_set_backlight(uint8_t brightness) {
  ledcWrite(PIN_LCD_BL, brightness);
}

void display_lvgl_init() {
  s_lcd.init();
  s_lcd.setRotation(0);
  s_lcd.fillScreen(0x0A0A1A);

  ledcAttach(PIN_LCD_BL, LCD_BL_FREQ, LCD_BL_RES);
  display_set_backlight(220);

  lv_init();

  lv_display_t* disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, lvgl_flush_cb);

  uint8_t* buf1 = (uint8_t*)heap_caps_malloc(DRAW_BUF_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  uint8_t* buf2 = (uint8_t*)heap_caps_malloc(DRAW_BUF_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!buf1) {
    buf1 = (uint8_t*)malloc(DRAW_BUF_BYTES);
  }
  lv_display_set_buffers(disp, buf1, buf2, DRAW_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);

  s_last_tick_ms = millis();
  Serial.println("[GFX] LovyanGFX ST7789 + LVGL 9.2 initialized.");
}

void display_lvgl_tick() {
  uint32_t now = millis();
  uint32_t elapsed = now - s_last_tick_ms;
  if (elapsed > 0) {
    lv_tick_inc(elapsed);
    s_last_tick_ms = now;
  }
  lv_timer_handler();
}
