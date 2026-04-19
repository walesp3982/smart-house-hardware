#pragma once
#include <Arduino.h>

void mqtt_setup(const char* uuid, const char* user, const char* pass, const char* chip_id);
void mqtt_loop(const char* chip_id);
void mqtt_publish_state(bool camera_active, const char* stream_url);