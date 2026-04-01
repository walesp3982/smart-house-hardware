#include <builder.h>

String JsonBuilder::jsonToString(JsonDocument &doc) {
    String data;
    serializeJson(doc, data);
    return data;
}

String JsonBuilder::doorState(bool state) {
    JsonDocument doc;
    doc["state"] = state ? "on" : "off";
    return JsonBuilder::jsonToString(doc);

}

String JsonBuilder::lightState(bool state) {
    JsonDocument doc;
    doc["state"] = state ? "on" : "off";
    return JsonBuilder::jsonToString(doc);
}

String JsonBuilder::temperatureState(bool state, uint8_t limit_temp, bool enable_auto) {
    JsonDocument doc;
    doc["state"] = state ? "on" : "off";
    doc["limit_temp"] = limit_temp;
    doc["enable_auto"] = enable_auto;
    return JsonBuilder::jsonToString(doc);
}

String JsonBuilder::movementState(bool state) {
    JsonDocument doc;
    doc["state"] = state ? "on" : "off";
    return JsonBuilder::jsonToString(doc);
}