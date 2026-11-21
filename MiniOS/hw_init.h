#pragma once

#include <Arduino.h>
#include "types.h"

extern volatile bool g_cam_ready;
extern volatile uint16_t g_cam_last_err;
extern volatile bool g_mic_ready;
extern volatile bool g_fs_ready;

void hw_init_early();
void hw_init_buttons();
ButtonId hw_poll_button();
void hw_inject_virtual_button(ButtonId btn);

void hw_init_fs();
void hw_init_camera_safe();
void hw_init_mic_safe();

bool hw_capture_jpeg(uint8_t** out_buf, size_t* out_len);
void hw_release_jpeg(uint8_t* buf);
size_t hw_read_mic_pcm16(int16_t* dst_samples, size_t max_samples);
