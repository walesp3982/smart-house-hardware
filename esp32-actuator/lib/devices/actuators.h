#pragma once
#include "Arduino.h"
#include "i2c_protocol.h"
#include "ArduinoJson.h"
#include "controller.h"

enum class TypeActuator : uint8_t
{
    DOOR = 0,
    LIGHT = 1
};

enum class StateActuator
{
    ON = 1,
    OFF = 0
};

class Actuator
{
public:
    String uuid;
    uint8_t bit_state;
    TypeActuator type;
    StateActuator state;

    Actuator(String _uuid, uint8_t _bit_state, TypeActuator _type)
        : uuid(_uuid), bit_state(_bit_state), type(_type), state(StateActuator::OFF) {}
};

constexpr uint8_t MAX_ACTUATORS = 8;

class ActuatorsController : public ArduinoController
{
private:
    Actuator *actuators[MAX_ACTUATORS];
    uint8_t size;
    Actuator *get_actuator_by_uuid(String uuid);

public:
    ActuatorsController(uint8_t _address, uint8_t node_id);
    void add_actuators(Actuator *actuator);
    // Relacionado a MQTT
    void subscriber_mqtt(String topic, JsonDocument doc);
    std::vector<Publish> publish_mqtt();
    std::vector<String> get_subscribe_topics();
    // Relacionado a i2c
    void state_device_i2c(I2CPacket &ptk);
    I2CPacket set_device_i2c();
};
