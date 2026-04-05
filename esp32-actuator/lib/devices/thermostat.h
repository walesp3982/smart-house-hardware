#include "controller.h"

class TemperatureController : public ArduinoController {
private:
    int8_t temp_limit = 0;
    bool state;
    bool enable_auto;
    String uuid;
public:
    TemperatureController(String _uuid, uint8_t _address, uint8_t _node_id);
    void subscriber_mqtt(String topic, JsonDocument doc);
    std::vector<Publish> publish_mqtt();
    std::vector<String> get_subscribe_topics();
    void state_device_i2c(I2CPacket &ptk);
    I2CPacket set_device_i2c();
};

