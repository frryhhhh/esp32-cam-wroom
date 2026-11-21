#include "ui_screens.h"
#include "config.h"
#include <lvgl.h>

static ScreenState s_screen_state = SCREEN_LOCK;
static ThemeConfig s_theme = {
  0x0B0F19,
  0x667EEA,
  0x161E31,
  0xF8FAFC,
  "Midnight Indigo"
};

static lv_obj_t* s_scr_lock = nullptr;
static lv_obj_t* s_island   = nullptr;
static lv_obj_t* s_island_lbl = nullptr;
static lv_obj_t* s_brand_lbl  = nullptr;
static lv_obj_t* s_clock_lbl  = nullptr;
static lv_obj_t* s_date_lbl   = nullptr;
static lv_obj_t* s_ap_lbl     = nullptr;

static lv_obj_t* s_scr_home = nullptr;
static lv_obj_t* s_home_header = nullptr;
static lv_obj_t* s_home_status = nullptr;
static lv_obj_t* s_tiles[6] = {nullptr};
static int s_selected_tile = 0;

static lv_color_t lv_hex(uint32_t hex) {
  return lv_color_hex(hex);
}

static void fade_in_obj(lv_obj_t* obj, uint32_t delay_ms, uint32_t duration_ms = 420) {
  if (!obj) return;
  lv_obj_set_style_opa(obj, LV_OPA_0, 0);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, obj);
  lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
  lv_anim_set_delay(&a, delay_ms);
  lv_anim_set_duration(&a, duration_ms);
  lv_anim_set_exec_cb(&a, [](void* var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
  });
  lv_anim_start(&a);
}

static void start_island_pulse() {
  if (!s_island) return;
  lv_anim_del(s_island, nullptr);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, s_island);
  lv_anim_set_values(&a, 148, 168);
  lv_anim_set_duration(&a, 1100);
  lv_anim_set_playback_delay(&a, 150);
  lv_anim_set_playback_duration(&a, 1100);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&a, [](void* var, int32_t w) {
    lv_obj_set_width((lv_obj_t*)var, (int32_t)w);
  });
  lv_anim_start(&a);
}

static void build_lock_screen() {
  s_scr_lock = lv_obj_create(nullptr);
  lv_obj_set_size(s_scr_lock, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_set_style_bg_color(s_scr_lock, lv_color_hex(0x0A0A1A), 0);
  lv_obj_set_style_bg_opa(s_scr_lock, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(s_scr_lock, 0, 0);
  lv_obj_clear_flag(s_scr_lock, LV_OBJ_FLAG_SCROLLABLE);

  // Капсула Dynamic Island сверху
  s_island = lv_obj_create(s_scr_lock);
  lv_obj_set_size(s_island, 156, 28);
  lv_obj_align(s_island, LV_ALIGN_TOP_MID, 0, 12);
  lv_obj_set_style_bg_color(s_island, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_island, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(s_island, 14, 0);
  lv_obj_set_style_border_width(s_island, 1, 0);
  lv_obj_set_style_border_color(s_island, lv_color_hex(0x2A2D45), 0);
  lv_obj_set_style_pad_all(s_island, 0, 0);
  lv_obj_clear_flag(s_island, LV_OBJ_FLAG_SCROLLABLE);

  s_island_lbl = lv_label_create(s_island);
  lv_label_set_text(s_island_lbl, "WIFI/OFF");
  lv_obj_set_style_text_color(s_island_lbl, lv_color_hex(0xE2E8F0), 0);
  lv_obj_set_style_text_font(s_island_lbl, &lv_font_montserrat_12, 0);
  lv_obj_center(s_island_lbl);

  // Заголовок MiniOS фиолетовым (#667eea)
  s_brand_lbl = lv_label_create(s_scr_lock);
  lv_label_set_text(s_brand_lbl, "MiniOS");
  lv_obj_align(s_brand_lbl, LV_ALIGN_TOP_MID, 0, 56);
  lv_obj_set_style_text_color(s_brand_lbl, lv_color_hex(0x667EEA), 0);
  lv_obj_set_style_text_font(s_brand_lbl, &lv_font_montserrat_28, 0);

  // Часы и дата по центру
  s_clock_lbl = lv_label_create(s_scr_lock);
  lv_label_set_text(s_clock_lbl, "10:21");
  lv_obj_align(s_clock_lbl, LV_ALIGN_CENTER, 0, -6);
  lv_obj_set_style_text_color(s_clock_lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(s_clock_lbl, &lv_font_montserrat_28, 0);

  s_date_lbl = lv_label_create(s_scr_lock);
  lv_label_set_text(s_date_lbl, "Sat, Nov 21");
  lv_obj_align(s_date_lbl, LV_ALIGN_CENTER, 0, 26);
  lv_obj_set_style_text_color(s_date_lbl, lv_color_hex(0x94A3B8), 0);
  lv_obj_set_style_text_font(s_date_lbl, &lv_font_montserrat_14, 0);

  lv_obj_t* hint_lbl = lv_label_create(s_scr_lock);
  lv_label_set_text(hint_lbl, "Press OK to unlock");
  lv_obj_align(hint_lbl, LV_ALIGN_BOTTOM_MID, 0, -42);
  lv_obj_set_style_text_color(hint_lbl, lv_color_hex(0x64748B), 0);
  lv_obj_set_style_text_font(hint_lbl, &lv_font_montserrat_12, 0);
  fade_in_obj(hint_lbl, 360);

  // Нижняя строка AP: MiniOS-Setup | Pass: ahiralom
  s_ap_lbl = lv_label_create(s_scr_lock);
  lv_label_set_text(s_ap_lbl, "AP: MiniOS-Setup | Pass: ahiralom");
  lv_obj_align(s_ap_lbl, LV_ALIGN_BOTTOM_MID, 0, -14);
  lv_obj_set_style_text_color(s_ap_lbl, lv_color_hex(0xCBD5E1), 0);
  lv_obj_set_style_text_font(s_ap_lbl, &lv_font_montserrat_12, 0);
}

static void highlight_selected_tile() {
  for (int i = 0; i < 6; i++) {
    if (!s_tiles[i]) continue;
    bool sel = (i == s_selected_tile);
    lv_obj_set_style_border_width(s_tiles[i], sel ? 2 : 1, 0);
    lv_obj_set_style_border_color(s_tiles[i], sel ? lv_hex(s_theme.accentHex) : lv_color_hex(0x283248), 0);
  }
}

static void build_home_screen() {
  s_scr_home = lv_obj_create(nullptr);
  lv_obj_set_size(s_scr_home, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_set_style_bg_color(s_scr_home, lv_hex(s_theme.bgHex), 0);
  lv_obj_set_style_bg_opa(s_scr_home, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(s_scr_home, 0, 0);
  lv_obj_clear_flag(s_scr_home, LV_OBJ_FLAG_SCROLLABLE);

  s_home_header = lv_label_create(s_scr_home);
  lv_label_set_text(s_home_header, "MiniOS Home");
  lv_obj_align(s_home_header, LV_ALIGN_TOP_LEFT, 14, 12);
  lv_obj_set_style_text_color(s_home_header, lv_hex(s_theme.accentHex), 0);
  lv_obj_set_style_text_font(s_home_header, &lv_font_montserrat_20, 0);

  s_home_status = lv_label_create(s_scr_home);
  lv_label_set_text(s_home_status, "192.168.4.1 | CAM:-- MIC:--");
  lv_obj_align(s_home_status, LV_ALIGN_TOP_LEFT, 14, 38);
  lv_obj_set_style_text_color(s_home_status, lv_color_hex(0x94A3B8), 0);
  lv_obj_set_style_text_font(s_home_status, &lv_font_montserrat_12, 0);

  const char* titles[6] = {"Camera", "Audio", "Files", "Wi-Fi", "OTA", "Theme"};
  const char* subs[6]   = {"MJPEG", "16kHz", "FS", "AP+STA", "Update", "Style"};

  const int cols = 2;
  const int tile_w = 102;
  const int tile_h = 72;
  const int gap_x = 12;
  const int gap_y = 12;
  const int start_x = 14;
  const int start_y = 62;

  for (int i = 0; i < 6; i++) {
    int col = i % cols;
    int row = i / cols;
    int x = start_x + col * (tile_w + gap_x);
    int y = start_y + row * (tile_h + gap_y);

    lv_obj_t* card = lv_obj_create(s_scr_home);
    lv_obj_set_size(card, tile_w, tile_h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_hex(s_theme.cardHex), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t_lbl = lv_label_create(card);
    lv_label_set_text(t_lbl, titles[i]);
    lv_obj_align(t_lbl, LV_ALIGN_TOP_LEFT, 0, 2);
    lv_obj_set_style_text_color(t_lbl, lv_hex(s_theme.textHex), 0);
    lv_obj_set_style_text_font(t_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t* s_lbl = lv_label_create(card);
    lv_label_set_text(s_lbl, subs[i]);
    lv_obj_align(s_lbl, LV_ALIGN_BOTTOM_LEFT, 0, -2);
    lv_obj_set_style_text_color(s_lbl, lv_hex(s_theme.accentHex), 0);
    lv_obj_set_style_text_font(s_lbl, &lv_font_montserrat_12, 0);

    s_tiles[i] = card;
  }

  lv_obj_t* foot = lv_label_create(s_scr_home);
  lv_label_set_text(foot, "BACK -> Lock Screen");
  lv_obj_align(foot, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_color(foot, lv_color_hex(0x64748B), 0);
  lv_obj_set_style_text_font(foot, &lv_font_montserrat_12, 0);

  highlight_selected_tile();
}

static void play_lock_cascade() {
  fade_in_obj(s_island, 40, 350);
  fade_in_obj(s_brand_lbl, 150, 420);
  fade_in_obj(s_clock_lbl, 260, 460);
  fade_in_obj(s_date_lbl, 340, 460);
  fade_in_obj(s_ap_lbl, 450, 500);
  start_island_pulse();
}

void ui_init(const ThemeConfig& initialTheme) {
  s_theme = initialTheme;
  build_lock_screen();
  build_home_screen();
  ui_show_lock_screen();
}

void ui_show_lock_screen() {
  s_screen_state = SCREEN_LOCK;
  if (s_scr_lock) {
    lv_screen_load(s_scr_lock);
    play_lock_cascade();
  }
}

void ui_show_home_screen() {
  s_screen_state = SCREEN_HOME;
  if (s_scr_home) {
    ui_apply_theme(s_theme);
    lv_screen_load(s_scr_home);
  }
}

ScreenState ui_get_screen_state() {
  return s_screen_state;
}

const ThemeConfig& ui_get_theme() {
  return s_theme;
}

void ui_apply_theme(const ThemeConfig& theme) {
  s_theme = theme;
  if (!s_scr_home) return;

  lv_obj_set_style_bg_color(s_scr_home, lv_hex(s_theme.bgHex), 0);
  if (s_home_header) {
    lv_obj_set_style_text_color(s_home_header, lv_hex(s_theme.accentHex), 0);
  }

  for (int i = 0; i < 6; i++) {
    if (!s_tiles[i]) continue;
    lv_obj_set_style_bg_color(s_tiles[i], lv_hex(s_theme.cardHex), 0);
    lv_obj_t* child0 = lv_obj_get_child(s_tiles[i], 0);
    lv_obj_t* child1 = lv_obj_get_child(s_tiles[i], 1);
    if (child0) lv_obj_set_style_text_color(child0, lv_hex(s_theme.textHex), 0);
    if (child1) lv_obj_set_style_text_color(child1, lv_hex(s_theme.accentHex), 0);
  }
  highlight_selected_tile();
}

void ui_handle_button(ButtonId btn) {
  if (btn == BTN_NONE) return;

  if (s_screen_state == SCREEN_LOCK) {
    if (btn == BTN_OK) {
      ui_show_home_screen();
    }
    return;
  }

  // На рабочем столе
  if (btn == BTN_BACK) {
    ui_show_lock_screen();
    return;
  }

  if (btn == BTN_RIGHT) {
    s_selected_tile = (s_selected_tile + 1) % 6;
    highlight_selected_tile();
  } else if (btn == BTN_LEFT) {
    s_selected_tile = (s_selected_tile + 5) % 6;
    highlight_selected_tile();
  } else if (btn == BTN_DOWN) {
    s_selected_tile = (s_selected_tile + 2) % 6;
    highlight_selected_tile();
  } else if (btn == BTN_UP) {
    s_selected_tile = (s_selected_tile + 4) % 6;
    highlight_selected_tile();
  } else if (btn == BTN_OK) {
    // Быстрое переключение темы при выборе плитки Theme (индекс 5)
    if (s_selected_tile == 5) {
      if (s_theme.accentHex == 0x667EEA) {
        s_theme = {0x0D1B1E, 0x2EC4B6, 0x162A2F, 0xF1FAEE, "Emerald Dark"};
      } else if (s_theme.accentHex == 0x2EC4B6) {
        s_theme = {0x1A1018, 0xF72585, 0x2B1927, 0xFFF0F6, "Neon Rose"};
      } else {
        s_theme = {0x0B0F19, 0x667EEA, 0x161E31, 0xF8FAFC, "Midnight Indigo"};
      }
      ui_apply_theme(s_theme);
    }
  }
}

void ui_update_dynamic_island(bool wifiConnected, const char* ipStr) {
  if (!s_island_lbl) return;
  char buf[40];
  if (wifiConnected && ipStr && ipStr[0] != '\0') {
    snprintf(buf, sizeof(buf), "WIFI/ON %s", ipStr);
  } else {
    snprintf(buf, sizeof(buf), "WIFI/OFF");
  }
  lv_label_set_text(s_island_lbl, buf);
}

void ui_update_clock_str(const char* timeStr, const char* dateStr) {
  if (s_clock_lbl && timeStr) {
    lv_label_set_text(s_clock_lbl, timeStr);
  }
  if (s_date_lbl && dateStr) {
    lv_label_set_text(s_date_lbl, dateStr);
  }
}

void ui_update_status_badges(const SystemStatus& st) {
  if (!s_home_status) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "192.168.4.1 | CAM:%s MIC:%s",
           st.camReady ? "OK" : "OFF",
           st.micReady ? "OK" : "OFF");
  lv_label_set_text(s_home_status, buf);
}
