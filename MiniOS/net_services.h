#pragma once

#include <Arduino.h>
#include "types.h"

void net_load_configs(ThemeConfig& theme, WiFiCreds& wifiCfg);
void net_save_theme(const ThemeConfig& theme);
void net_save_wifi(const WiFiCreds& wifiCfg);

void net_init_wifi_and_server(const WiFiCreds& wifiCfg);
void net_loop();
void net_update_ntp_and_clock(char* outTimeStr, size_t timeLen, char* outDateStr, size_t dateLen, bool* outNtpSynced);
bool net_is_sta_connected();
