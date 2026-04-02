#include <thermostat.h>
#include <builder.h>
TemperatureController::TemperatureController(String _uuid, uint8_t _address, uint8_t _node_id) {
    uuid = _uuid;
    address_i2c = _address;
    node_id = _node_id;
}

void TemperatureController::subscriber_mqtt(String topic, JsonDocument doc) {
    if(!is_topic_set_device(topic, uuid)) {
        return;
    }
    bool has_enable_auto = !doc["enable_auto"].isNull();
    if (has_enable_auto) {
        enable_auto = doc["enable_auto"].as<bool>();
    }

    bool has_action = !doc["action"].isNull();
    if (has_action) {
        has_action = doc["action"].as<bool>();
    }

    bool has_limit = !doc["temperature_limit"].isNull();
    if (has_limit) {
        temp_limit = static_cast<uint8_t>(doc["temperature_limit"].as<int>());
    }
}

std::vector<Publish> TemperatureController::publish_mqtt() {
    std::vector<Publish> publish;

    String response = JsonBuilder::temperatureState(state, temp_limit, enable_auto);
    String topic = "/" + uuid + "/value";
    Publish p = {topic, response};
    publish.push_back(p);
    return publish;
}

I2CPacket TemperatureController::set_device_i2c() {
    I2CPacket pkt;
    uint8_t data = 0x00;
    
    uint8_t state_bit = (state) ? 0x40 : 0x00;
    uint8_t auto_bit = enable_auto ? 0x80 : 0x00;
    data = auto_bit | state_bit | (temp_limit & 0x03F);
    pkt.node_id = node_id;
    pkt.cmd = CMD_SET;
    pkt.data = data;
    pkt.checksum = pkt_checksum(pkt);

    return pkt;
}

void TemperatureController::state_device_i2c(I2CPacket &pkt) {
    // TODO: CORREGIR PARA AGREGAR LA TEMPERATURA
    if (pkt.cmd == CMD_SET) {
        enable_auto = (pkt.data >> 7) & 0x01;
        temp_limit = pkt.data & 0x3F;
        state = (pkt.data >> 6 ) & 0x01;
    }
}