#pragma once

#include <Arduino.h>
#include "types.h"

void ui_init(const ThemeConfig& initialTheme);
void ui_show_lock_screen();
void ui_show_home_screen();
ScreenState ui_get_screen_state();

void ui_apply_theme(const ThemeConfig& theme);
const ThemeConfig& ui_get_theme();

void ui_handle_button(ButtonId btn);
void ui_update_dynamic_island(bool wifiConnected, const char* ipStr);
void ui_update_clock_str(const char* timeStr, const char* dateStr);
void ui_update_status_badges(const SystemStatus& st);
