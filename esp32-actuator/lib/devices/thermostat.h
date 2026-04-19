#include "controller.h"

class TemperatureController : public ArduinoController {
private:
    int8_t temp_limit = 50;
    bool state;
    bool enable_auto;
    String uuid;
public:
    TemperatureController(String _uuid, uint8_t _address, uint8_t _node_id);
    void subscriber_mqtt(String topic, JsonDocument doc);
    std::vector<Publish> publish_mqtt();
    std::vector<String> get_subscribe_topics();
    void state_device_i2c(I2CPacket &ptk);
    void change_state(uint8_t _temp_timit, bool _state, bool _enable_auto) {
        temp_limit = temp_limit;
        state = _state;
        enable_auto = _enable_auto;
    }
    uint8_t get_temp_timit() {
        return temp_limit;
    }
    bool get_enable_auto() {
        return enable_auto;
    }

    bool get_state() {
        return state;
    }
    I2CPacket set_device_i2c();
};

