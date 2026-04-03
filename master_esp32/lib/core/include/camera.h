#pragma once
#include "esp_camera.h"

bool     camera_init();
void     camera_deinit();
bool     camera_is_active();
void     camera_set_active(bool active);
camera_fb_t* camera_capture();   // captura un frame, caller debe llamar esp_camera_fb_return()