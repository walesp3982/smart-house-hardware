#include <Core.h>
#include <Messenger.h>
/*
    Implementación de la clase Gate
*/

Gate::Gate(uint8_t _pin, int _rotation, Data _data, uint8_t _i2c_command)
    :  motor(), pin(_pin), rotation(_rotation)
{
    bit_data_i2c = _i2c_command;
    status = Status::OFF;
    data = _data;
}

void Gate::init()
{
    motor.attach(pin);
}
void Gate::activate()
{
    motor.write(0);
    status = Status::ON;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_SUCCESS, data.code, "Abierto correctamente")
    );
}

void Gate::desactivate()
{
    motor.write(rotation);
    status = Status::OFF;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_ERROR, data.code, "Cerrado correctamente")
    );
}
