#include <controller.h>

bool is_topic_set_device(String topic, String uuid) {
    return "/"+uuid+"/set" == topic;
}

String generate_set_topic(String uuid) {
    return String("/") + uuid + "/set"; 
}
DevicesController::DevicesController(): size(0) {
}

void DevicesController::add_arduino(ArduinoController *controller) {
    if(size >= MAX_DEVICES) {
        return;
    }

    arduinos[size++] = controller;
}
void DevicesController::subscriber_action_mqtt(String topic, JsonDocument doc) {
    for(int i = 0; i < size; i++) {
        ArduinoController* arduino = arduinos[i];
        
        arduino->subscriber_mqtt(topic, doc);
    }
}

std::vector<Publish> DevicesController::publish_action_mqtt() {
    std::vector<Publish> all_publish;

    for(int i = 0; i < size; i++) {
        ArduinoController *arduino = arduinos[i];
        if (!arduino) {
            continue;
        }
        std::vector<Publish> arduino_publish = arduino->publish_mqtt();

        all_publish.insert(all_publish.end(), arduino_publish.begin(), arduino_publish.end());
    }
    return all_publish;
}

/**
 * Retorna un vector de direcciones i2c
 */
std::vector<I2CMetadata> DevicesController::address_nodes() {
    std::vector<I2CMetadata> list_address;
    for(int i = 0; i < size; i++) {
        ArduinoController* arduino = arduinos[i];
        I2CMetadata metadata(arduino->get_address_i2c(), arduino->get_node_id());
        list_address.push_back(metadata);
    }
    return list_address;
}

std::vector<I2CBoxing> DevicesController::send_i2c() {
    std::vector<I2CBoxing> list_updated_packed;

    for (int i = 0; i < size; i++) {
        ArduinoController* arduino = arduinos[i];
        I2CPacket pkt = arduino->set_device_i2c();
        uint8_t address = arduino->get_address_i2c();
        I2CBoxing boxing(pkt, address);
        list_updated_packed.push_back(boxing);
    }

    return list_updated_packed;
}

void DevicesController::received_i2c(std::vector<I2CPacket> &packets) {
    for (I2CPacket pkt: packets) {
        for (int i = 0; i < size; i++) {
            ArduinoController *arduino = arduinos[i];
            if (pkt.node_id == arduino->get_node_id()) {
                arduino->state_device_i2c(pkt);
            }
        }
    }
}

std::vector<String> DevicesController::get_topics_devices() {
    std::vector<String> topics;
    for (int i = 0; i < size; i++) {
        ArduinoController* arduino = arduinos[i];
        if (!arduino) {
            continue;
        }
        std::vector<String> topic_device = arduino->get_subscribe_topics() ;

        topics.insert(topics.end(), topic_device.begin(), topic_device.end());
    }

    return topics;
}