#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <i2c_protocol.h>
struct Publish {
    String topic;
    String payload;
};

class ArduinoController
{
protected:
    uint8_t node_id;
    uint8_t address_i2c; 
public:
    // Configuration de mqtt (obtención de valores y configuracion)
    virtual void subscriber_mqtt(String topic, JsonDocument doc) = 0;
    virtual std::vector<Publish> publish_mqtt() = 0;
    // Configuración de i2c
    virtual void state_device_i2c(I2CPacket &ptk) = 0;
    virtual I2CPacket set_device_i2c() = 0; 
    uint8_t get_address_i2c() {
        return address_i2c;
    }
};

static bool is_topic_set_device(String topic, String uuid) {
    return "/"+uuid == topic;
}