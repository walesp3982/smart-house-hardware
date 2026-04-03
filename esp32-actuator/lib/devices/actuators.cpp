#include "actuators.h"
#include "builder.h"

ActuatorsController::ActuatorsController(uint8_t _address, uint8_t _node_id)
{
    size = 0;
    address_i2c = _address;
    node_id = _node_id;
}

void ActuatorsController::add_actuators(Actuator* actuator)
{
    if (size >= MAX_ACTUATORS)
    {
        return;
    }
    actuators[size] = actuator;
    size++;
}

Actuator *ActuatorsController::get_actuator_by_uuid(String uuid)
{
    for (int i = 0; i < size; i++)
    {
        Actuator *actuator = actuators[i];
        if (actuator->uuid == uuid)
        {
            return actuator;
        }
    }
    return nullptr;
}

void ActuatorsController::subscriber_mqtt(String topic, JsonDocument doc)
{
    for (int i = 0; i < size; i++)
    {
        Actuator *actuator = actuators[i];
        if (!actuator)
        {
            break;
        }
        if (is_topic_set_device(topic, actuator->uuid))
        {
            const char *action = doc["action"];
            if (!action)
            {
                return;
            }
            bool turn_on = strcmp(action, "on") == 0;
            actuator->state = turn_on ? StateActuator::ON : StateActuator::OFF;
        }
    }
}

std::vector<Publish> ActuatorsController::publish_mqtt()
{
    std::vector<Publish> publish;

    for (int i = 0; i < size; i++)
    {
        Actuator *actuator = actuators[i];

        if (!actuator)
        {
            continue;
        }

        String data = "";
        bool turn_on = actuator->state == StateActuator::ON ? true : false;
        switch (actuator->type)
        {
        case TypeActuator::DOOR:
            /* code */
            data = JsonBuilder::doorState(turn_on);
            break;
        case TypeActuator::LIGHT:
            data = JsonBuilder::lightState(turn_on);
        }

        String topic = "/" + actuator->uuid + "/value";
        Publish p = {topic, data};
        publish.push_back(p);
    }

    return publish;
}

void ActuatorsController::state_device_i2c(I2CPacket &ptk)
{
    for (int i = 0; i < size; i++)
    {
        Actuator *actuator = actuators[i];
        if (!actuator)
        {
            continue;
        }

        bool turn_on = (1 >> actuator->bit_state) == 0x01 ? true : false;
        actuator->state = turn_on ? StateActuator::ON : StateActuator::OFF;
    }
}

I2CPacket ActuatorsController::set_device_i2c()
{
    uint8_t data = 0x00;

    for (int i = 0; i < size; i++)
    {
        Actuator *actuator = actuators[i];
        if (!actuator)
        {
            continue;
        }
        bool turn_on = actuator->state == StateActuator::ON ? true : false;
        if (turn_on)
        {
            data |= (1 << actuator->bit_state);
        }
        else
        {
            data &= ~(1 << actuator->bit_state);
        }
    }

    I2CPacket pkt;
    pkt.node_id = node_id;
    pkt.cmd = CMD_SET;
    pkt.data = data;
    pkt.checksum = pkt_checksum(pkt);

    return pkt;
}