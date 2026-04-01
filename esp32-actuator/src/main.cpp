#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "i2c_protocol.h"
#include "generated_devices.h"

enum class DeviceType : uint8_t {
    Light,
    Door,
    Thermostat,
    Movement,
    Camera,
    Unknown
};

struct BridgeDevice {
    const char* uuid;
    DeviceType type;
    uint8_t i2c_addr;
    uint8_t node_id;
    int8_t bit;
};

static constexpr BridgeDevice BRIDGE_DEVICES[] = {
    {UUID_PUERTA_PRINCIPAL, DeviceType::Door, 0x09, 2, 0},
    {UUID_PUERTA_GARAGE, DeviceType::Door, 0x09, 2, 1},
    {UUID_PUERTA_DORMITORIO, DeviceType::Door, 0x09, 2, 2},
    {UUID_LUZ_GARAGE, DeviceType::Light, 0x09, 2, 3},
    {UUID_LUZ_DORMITORIO, DeviceType::Light, 0x09, 2, 4},
    {UUID_LUZ_SALA, DeviceType::Light, 0x09, 2, 5},
    {UUID_LUZ_COCINA, DeviceType::Light, 0x09, 2, 6},
    {UUID_SENSOR_TEMPERATURE, DeviceType::Thermostat, 0x0A, 3, -1},
    {UUID_SENSOR_MOVIMIENTO, DeviceType::Movement, 0x08, 1, -1},
};

static constexpr size_t BRIDGE_DEVICE_COUNT = sizeof(BRIDGE_DEVICES) / sizeof(BridgeDevice);
static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static char mqtt_client_id[32] = {0};
static unsigned long last_status_poll = 0;
static uint8_t actuator_cache_mask = 0;

static void init_mqtt_client_id() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(
        mqtt_client_id,
        sizeof(mqtt_client_id),
        "esp32-%02X%02X%02X%02X%02X%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
}

static bool topic_matches_uuid(const char* topic, const char* uuid) {
    return topic[0] == '/' && strcmp(topic + 1, uuid) == 0;
}

static bool i2c_write_packet(uint8_t addr, const I2CPacket& pkt) {
    Wire.beginTransmission(addr);
    Wire.write(reinterpret_cast<const uint8_t*>(&pkt), PKT_SIZE);
    return Wire.endTransmission() == 0;
}

static bool i2c_read_packet(uint8_t addr, I2CPacket& pkt) {
    if (Wire.requestFrom(addr, static_cast<uint8_t>(PKT_SIZE)) != PKT_SIZE) {
        return false;
    }
    pkt.node_id = Wire.read();
    pkt.cmd = Wire.read();
    pkt.data = Wire.read();
    pkt.checksum = Wire.read();
    return pkt_valid(pkt);
}

static bool request_node_status(uint8_t addr, uint8_t node_id, I2CPacket& response) {
    I2CPacket request{node_id, CMD_STATUS, 0x00, 0x00};
    request.checksum = pkt_checksum(request);
    if (!i2c_write_packet(addr, request)) {
        return false;
    }
    delay(10);
    if (!i2c_read_packet(addr, response)) {
        return false;
    }
    return response.node_id == node_id;
}

static void publish_json(const char* topic, const JsonDocument& doc, bool retained = true) {
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    mqtt.publish(topic, payload, retained);
}

static void publish_actuator_states(uint8_t mask) {
    for (const auto& device : BRIDGE_DEVICES) {
        if (device.type != DeviceType::Light && device.type != DeviceType::Door) {
            continue;
        }
        bool state = (mask >> device.bit) & 0x01;
        StaticJsonDocument<64> doc;
        doc["state"] = state ? "on" : "off";
        char topic[128];
        snprintf(topic, sizeof(topic), "/%s/value", device.uuid);
        publish_json(topic, doc);
    }
}

static void publish_temperature_state(bool state, uint8_t limit_temp, bool enable_auto) {
    StaticJsonDocument<128> doc;
    doc["state"] = state ? "on" : "off";
    doc["limit_temp"] = limit_temp;
    doc["enable_auto"] = enable_auto;
    char topic[128];
    snprintf(topic, sizeof(topic), "/%s/value", UUID_SENSOR_TEMPERATURE);
    publish_json(topic, doc);
}

static void publish_movement_state(bool state) {
    StaticJsonDocument<64> doc;
    doc["state"] = state ? "on" : "off";
    char topic[128];
    snprintf(topic, sizeof(topic), "/%s/value", UUID_SENSOR_MOVIMIENTO);
    publish_json(topic, doc);
}

static bool read_actuator_status(uint8_t& mask) {
    I2CPacket response;
    if (!request_node_status(0x09, 2, response)) {
        return false;
    }
    mask = response.data;
    return true;
}

struct ThermostatStatus {
    bool state_on;
    bool enable_auto;
    uint8_t limit_temp;
};

static bool read_thermostat_status(ThermostatStatus& status) {
    I2CPacket response;
    if (!request_node_status(0x0A, 3, response)) {
        return false;
    }
    status.enable_auto = (response.data >> 7) & 0x01;
    status.state_on = (response.data >> 6) & 0x01;
    status.limit_temp = response.data & 0x3F;
    return true;
}

static bool read_movement_status(bool& state) {
    I2CPacket response;
    if (!request_node_status(0x08, 1, response)) {
        return false;
    }
    state = response.data == 0x01;
    return true;
}

static void publish_all_states() {
    uint8_t actuator_mask;
    if (read_actuator_status(actuator_mask)) {
        actuator_cache_mask = actuator_mask;
        publish_actuator_states(actuator_mask);
    }
    ThermostatStatus thermostat;
    if (read_thermostat_status(thermostat)) {
        publish_temperature_state(thermostat.state_on, thermostat.limit_temp, thermostat.enable_auto);
    }
    bool movement_state;
    if (read_movement_status(movement_state)) {
        publish_movement_state(movement_state);
    }
}

static void publish_device_state(const BridgeDevice& device) {
    switch (device.type) {
        case DeviceType::Light:
        case DeviceType::Door: {
            uint8_t mask;
            if (!read_actuator_status(mask)) {
                return;
            }
            bool state = (mask >> device.bit) & 0x01;
            StaticJsonDocument<64> doc;
            doc["state"] = state ? "on" : "off";
            char topic[128];
            snprintf(topic, sizeof(topic), "/%s/value", device.uuid);
            publish_json(topic, doc);
            break;
        }
        case DeviceType::Thermostat: {
            ThermostatStatus thermostat;
            if (!read_thermostat_status(thermostat)) {
                return;
            }
            publish_temperature_state(thermostat.state_on, thermostat.limit_temp, thermostat.enable_auto);
            break;
        }
        case DeviceType::Movement: {
            bool state;
            if (!read_movement_status(state)) {
                return;
            }
            publish_movement_state(state);
            break;
        }
        default:
            break;
    }
}

static bool send_actuator_command(int8_t bit, bool turn_on) {
    uint8_t mask;
    if (!read_actuator_status(mask)) {
        mask = actuator_cache_mask;
    }
    if (turn_on) {
        mask |= (1 << bit);
    } else {
        mask &= ~(1 << bit);
    }
    I2CPacket request{2, CMD_SET, mask, 0x00};
    request.checksum = pkt_checksum(request);
    if (!i2c_write_packet(0x09, request)) {
        return false;
    }
    delay(10);
    actuator_cache_mask = mask;
    return true;
}

static bool send_thermostat_command(bool has_enable_auto, bool enable_auto, bool has_action, bool action_on, bool has_limit, uint8_t limit_temp) {
    ThermostatStatus current{false, true, 0};
    read_thermostat_status(current);
    if (!has_enable_auto) {
        enable_auto = current.enable_auto;
    }
    if (!has_action) {
        action_on = current.state_on;
    }
    if (!has_limit) {
        limit_temp = current.limit_temp;
    }
    if (limit_temp > 63) {
        limit_temp = 63;
    }
    uint8_t data = 0;
    if (enable_auto) {
        data |= 0x80;
    }
    if (action_on) {
        data |= 0x40;
    }
    data |= (limit_temp & 0x3F);
    I2CPacket request{3, CMD_SET, data, 0x00};
    request.checksum = pkt_checksum(request);
    return i2c_write_packet(0x0A, request);
}

static bool send_movement_command(bool turn_on) {
    I2CPacket request{1, CMD_SET, turn_on ? 0x01 : 0x00, 0x00};
    request.checksum = pkt_checksum(request);
    return i2c_write_packet(0x08, request);
}

static void mqtt_message_callback(char* topic, byte* payload, unsigned int length) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.printf("[MQTT] JSON inv�lido en %s: %s\n", topic, error.c_str());
        return;
    }

    for (const auto& device : BRIDGE_DEVICES) {
        if (!topic_matches_uuid(topic, device.uuid)) {
            continue;
        }
        switch (device.type) {
            case DeviceType::Light:
            case DeviceType::Door: {
                const char* action = doc["action"];
                if (!action) {
                    return;
                }
                bool turn_on = strcmp(action, "on") == 0;
                send_actuator_command(device.bit, turn_on);
                publish_device_state(device);
                break;
            }
            case DeviceType::Thermostat: {
                bool has_enable_auto = !doc["enable_auto"].isNull();
                bool enable_auto = doc["enable_auto"].as<bool>();
                bool has_action = !doc["action"].isNull();
                bool action_on = false;
                if (has_action) {
                    const char* action = doc["action"];
                    action_on = strcmp(action, "on") == 0;
                }
                bool has_limit = !doc["temperature_limit"].isNull();
                uint8_t limit_temp = 0;
                if (has_limit) {
                    limit_temp = static_cast<uint8_t>(doc["temperature_limit"].as<int>());
                }
                send_thermostat_command(has_enable_auto, enable_auto, has_action, action_on, has_limit, limit_temp);
                publish_device_state(device);
                break;
            }
            case DeviceType::Movement: {
                const char* action = doc["action"];
                if (!action) {
                    return;
                }
                bool turn_on = strcmp(action, "on") == 0;
                send_movement_command(turn_on);
                publish_device_state(device);
                break;
            }
            default:
                break;
        }
        return;
    }
}

static void mqtt_connect() {
    while (!mqtt.connected()) {
        if (mqtt.connect(mqtt_client_id, MQTT_USER, MQTT_PASSWORD)) {
            for (const auto& device : BRIDGE_DEVICES) {
                char topic[100];
                snprintf(topic, sizeof(topic), "/%s", device.uuid);
                mqtt.subscribe(topic);
            }
            publish_all_states();
            return;
        }
        delay(3000);
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    init_mqtt_client_id();
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqtt_message_callback);
    mqtt_connect();
}

void loop() {
    if (!mqtt.connected()) {
        mqtt_connect();
    }
    mqtt.loop();
    if (millis() - last_status_poll > 30000) {
        publish_all_states();
        last_status_poll = millis();
    }
}
