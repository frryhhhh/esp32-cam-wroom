#pragma once

#include <Arduino.h>

enum ButtonId : uint8_t {
  BTN_NONE = 0,
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_OK,
  BTN_BACK
};

enum ScreenState : uint8_t {
  SCREEN_LOCK = 0,
  SCREEN_HOME
};

struct ThemeConfig {
  uint32_t bgHex;        // Фон рабочего стола / акцентов (например 0x0B0F19)
  uint32_t accentHex;    // Акцентный цвет (например 0x667EEA)
  uint32_t cardHex;      // Карточки / иконки (например 0x161D2E)
  uint32_t textHex;      // Основной текст (например 0xF8FAFC)
  char themeName[24];    // Название темы
};

struct WiFiCreds {
  char ssid[33];
  char password[65];
  bool enabled;
};

struct SystemStatus {
  bool fsReady;
  bool camReady;
  bool micReady;
  bool wifiStaConnected;
  bool ntpSynced;
  uint16_t camErrorCode;
  int staClients;
  uint32_t freeHeap;
  uint32_t freePsram;
};
