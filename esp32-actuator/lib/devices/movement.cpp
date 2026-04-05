#include<movement.h>
#include<builder.h>

MoveController::MoveController(String _uuid, uint8_t _address, uint8_t _node_id)
: uuid(_uuid) {
    address_i2c = _address;
    node_id = _node_id;
}

void MoveController::subscriber_mqtt(String topic, JsonDocument doc) {
    if(!is_topic_set_device(topic, uuid)) {
        return;
    }


    const char* action = doc["action"];
    if (!action) {
        return;
    }

    if (strcmp(action, "on") == 0) {
        state = true;
    } else if (strcmp(action, "off") == 0) {
        state = false;
    }
}

std::vector<Publish> MoveController::publish_mqtt() {
    std::vector<Publish> publish;

    String data = JsonBuilder::movementState(state);

    String topic = "/" + uuid;

    Publish p{topic, data};

    publish.push_back(p);
    return publish;
}

std::vector<String> MoveController::get_subscribe_topics() {
    std::vector<String> topics;

    topics.push_back(generate_set_topic(uuid));
    return topics;
}
I2CPacket MoveController::set_device_i2c() {
    I2CPacket pkt;

    uint8_t data = state ? 0x01 : 0x00;

    pkt.node_id = node_id;
    pkt.cmd = CMD_SET;
    pkt.data = data;
    pkt.checksum = pkt_checksum(pkt);

    return pkt;
}


void MoveController::state_device_i2c(I2CPacket &ptk) {
    state = (ptk.data == 0x01) ? true : false;
}