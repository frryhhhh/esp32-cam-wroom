#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "types.h"
#include "hw_init.h"
#include "display_lvgl.h"
#include "ui_screens.h"
#include "net_services.h"

static ThemeConfig s_theme;
static WiFiCreds   s_wifi_cfg;
static uint32_t    s_last_ui_sync_ms = 0;

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\n=== Booting MiniOS (ESP32-S3 N16R8) ===");

  hw_init_early();
  hw_init_buttons();
  hw_init_fs();

  net_load_configs(s_theme, s_wifi_cfg);

  display_lvgl_init();
  ui_init(s_theme);
  display_lvgl_tick();

  // Безопасная инициализация камеры и микрофона (не валят систему при ошибках)
  hw_init_camera_safe();
  hw_init_mic_safe();

  net_init_wifi_and_server(s_wifi_cfg);

  SystemStatus st = {
    g_fs_ready,
    g_cam_ready,
    g_mic_ready,
    net_is_sta_connected(),
    false,
    g_cam_last_err,
    (int)WiFi.softAPgetStationNum(),
    ESP.getFreeHeap(),
    ESP.getFreePsram()
  };
  ui_update_status_badges(st);
  ui_update_dynamic_island(net_is_sta_connected(), WiFi.localIP().toString().c_str());

  Serial.println("=== MiniOS Ready ===");
}

void loop() {
  net_loop();

  ButtonId btn = hw_poll_button();
  if (btn != BTN_NONE) {
    ThemeConfig prevTheme = ui_get_theme();
    ui_handle_button(btn);
    const ThemeConfig& curTheme = ui_get_theme();
    if (curTheme.accentHex != prevTheme.accentHex) {
      net_save_theme(curTheme);
    }
  }

  uint32_t now = millis();
  if ((now - s_last_ui_sync_ms) >= 1000) {
    s_last_ui_sync_ms = now;

    char tStr[16] = {0};
    char dStr[24] = {0};
    bool ntpOk = false;
    net_update_ntp_and_clock(tStr, sizeof(tStr), dStr, sizeof(dStr), &ntpOk);
    ui_update_clock_str(tStr, dStr);

    bool staOk = net_is_sta_connected();
    String ipText = staOk ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    ui_update_dynamic_island(staOk, ipText.c_str());

    SystemStatus st = {
      g_fs_ready,
      g_cam_ready,
      g_mic_ready,
      staOk,
      ntpOk,
      g_cam_last_err,
      (int)WiFi.softAPgetStationNum(),
      ESP.getFreeHeap(),
      ESP.getFreePsram()
    };
    ui_update_status_badges(st);
  }

  display_lvgl_tick();
  delay(2);
}
