#pragma once

#include <Arduino.h>

// ==========================================
// ФЛАГИ СБОРКИ И АППАРАТНОЙ ЗАЩИТЫ
// ==========================================
// Закомментируйте строку ниже, если шлейф OV2640 закорочен или требует отключения
#define ENABLE_CAMERA 1

// Микрофон INMP441 (I2S): пины 4, 5, 6 НЕ ИСПОЛЬЗУЮТСЯ (конфликт с камерой)
#define ENABLE_MIC_I2S 1

// ==========================================
// ПИНЫ УПРАВЛЕНИЯ И ВИРТУАЛЬНОГО GND
// ==========================================
static constexpr int PIN_VIRTUAL_GND = 3; // Физические GND/3V3 заняты -> виртуальный GND (OUTPUT LOW)

// Кнопки (INPUT_PULLUP, замыкание на GND)
static constexpr int PIN_BTN_UP    = 1;
static constexpr int PIN_BTN_DOWN  = 2;
static constexpr int PIN_BTN_LEFT  = 41;
static constexpr int PIN_BTN_RIGHT = 46;
static constexpr int PIN_BTN_OK    = 44;
static constexpr int PIN_BTN_BACK  = 43;

// ==========================================
// ДИСПЛЕЙ ST7789 240x320 (SPI + ШИМ BL)
// ==========================================
static constexpr int PIN_LCD_CS   = 14;
static constexpr int PIN_LCD_DC   = 45;
static constexpr int PIN_LCD_RST  = 48;
static constexpr int PIN_LCD_MOSI = 47;
static constexpr int PIN_LCD_SCLK = 21;
static constexpr int PIN_LCD_BL   = 42;

static constexpr uint16_t LCD_WIDTH  = 240;
static constexpr uint16_t LCD_HEIGHT = 320;
static constexpr uint32_t LCD_SPI_FREQ = 40000000;
static constexpr uint8_t  LCD_BL_CHANNEL = 0;
static constexpr uint32_t LCD_BL_FREQ = 5000;
static constexpr uint8_t  LCD_BL_RES = 8;

// ==========================================
// КАМЕРА OV2640 (FPC ШЛЕЙФ)
// ==========================================
static constexpr int PIN_CAM_XCLK  = 15;
static constexpr int PIN_CAM_SIOD  = 4;
static constexpr int PIN_CAM_SIOC  = 5;
static constexpr int PIN_CAM_Y2    = 11;
static constexpr int PIN_CAM_Y3    = 9;
static constexpr int PIN_CAM_Y4    = 8;
static constexpr int PIN_CAM_Y5    = 10;
static constexpr int PIN_CAM_Y6    = 12;
static constexpr int PIN_CAM_Y7    = 18;
static constexpr int PIN_CAM_Y8    = 17;
static constexpr int PIN_CAM_Y9    = 16;
static constexpr int PIN_CAM_VSYNC = 6;
static constexpr int PIN_CAM_HREF  = 7;
static constexpr int PIN_CAM_PCLK  = 13;
static constexpr int PIN_CAM_PWDN  = -1;
static constexpr int PIN_CAM_RESET = -1;

// ==========================================
// МИКРОФОН INMP441 (I2S MEMS, 16 кГц)
// ==========================================
static constexpr int PIN_I2S_WS  = 39;
static constexpr int PIN_I2S_SCK = 40;
static constexpr int PIN_I2S_SD  = 38;
static constexpr uint32_t MIC_SAMPLE_RATE = 16000;

// ==========================================
// СЕТЕВЫЕ ПАРАМЕТРЫ (ЖЁСТКИЕ ТРЕБОВАНИЯ)
// ==========================================
static constexpr const char* AP_SSID = "MiniOS-Setup";
static constexpr const char* AP_PASS = "ahiralom";
static constexpr IPAddress AP_LOCAL_IP(192, 168, 4, 1);
static constexpr IPAddress AP_GATEWAY(192, 168, 4, 1);
static constexpr IPAddress AP_SUBNET(255, 255, 255, 0);

static constexpr const char* NTP_SERVER_1 = "pool.ntp.org";
static constexpr const char* NTP_SERVER_2 = "time.nist.gov";
static constexpr long        GMT_OFFSET_SEC = 3 * 3600; // MSK по умолчанию, корректируется при необходимости
static constexpr int         DAYLIGHT_OFFSET_SEC = 0;

static constexpr const char* WIFI_CFG_PATH = "/wifi.json";
static constexpr const char* THEME_CFG_PATH = "/theme.json";
