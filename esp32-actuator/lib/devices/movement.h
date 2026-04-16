#include "Arduino.h"
#include "ArduinoJson.h"
#include "i2c_protocol.h"
#include "controller.h"

class MoveController : public ArduinoController {
private:
    String uuid;
    bool state;
public:
    MoveController(String _uuid, uint8_t _address, uint8_t _node_id); 
    void subscriber_mqtt(String topic, JsonDocument doc);
    std::vector<Publish> publish_mqtt();
    std::vector<String> get_subscribe_topics();
    void state_device_i2c(I2CPacket &pkt);
    I2CPacket set_device_i2c();
};