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
    uint8_t get_node_id() {
        return node_id;
    }
};

static bool is_topic_set_device(String topic, String uuid) {
    return "/"+uuid == topic;
}


struct I2CBoxing {
    I2CPacket pkt;
    uint8_t address;

    I2CBoxing(I2CPacket _pkt, uint8_t _address) : pkt(_pkt), address(_address) {}
};

struct I2CMetadata {
    uint8_t address;
    uint8_t node_id;

    I2CMetadata(uint8_t _address, uint8_t _node_id): address(_address), node_id(_node_id) {}
};
/**
 * Por el momento este es el máximo de devices en 
 * el dispositivo
 */
constexpr uint8_t MAX_DEVICES = 3;

class DevicesController {
private:
    ArduinoController* arduinos[MAX_DEVICES];
    uint8_t size;
public: 
    DevicesController();
    void add_arduino(ArduinoController &controller);
    void subscriber_action_mqtt(String topic, JsonDocument doc);
    std::vector<Publish> publish_action_mqtt();
    std::vector<I2CMetadata> address_nodes();
    std::vector<I2CBoxing> send_i2c();
    void received_i2c(std::vector<I2CPacket> &packets);
};