#pragma once
#include <Arduino.h>

void mqtt_setup(const char* uuid, const char* user, const char* pass);
void mqtt_loop();
void mqtt_publish_state(bool camera_active, const char* stream_url);