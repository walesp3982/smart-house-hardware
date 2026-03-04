#include <Core.h>

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
}

void Gate::desactivate()
{
    motor.write(0);
    status = Status::OFF;
}

void Gate::execute_command(char* order) {
    IDevice::execute_command(order);
}