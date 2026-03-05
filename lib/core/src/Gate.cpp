#include <Core.h>
#include <Messenger.h>
/*
    Implementación de la clase Gate
*/

Gate::Gate(uint8_t _pin, int _rotation, Data _data)
    :  motor(), pin(_pin), rotation(_rotation)
{
    status = Status::OFF;
    data = _data;
}

void Gate::init()
{
    motor.attach(pin);
}
void Gate::activate()
{
    motor.write(rotation);
    status = Status::ON;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_SUCCESS, data.code, "Abierto correctamente")
    );
}

void Gate::desactivate()
{
    motor.write(0);
    status = Status::OFF;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_ERROR, data.code, "Cerrado correctamente")
    );
}
