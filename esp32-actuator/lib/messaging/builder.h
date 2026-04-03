#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
class JsonBuilder
{
public:
    inline static String jsonToString(JsonDocument &doc);
    static String doorState(bool state);
    static String lightState(bool state);
    static String temperatureState(bool state, uint8_t limit_temp, bool enable_auto);
    static String movementState(bool state);
};