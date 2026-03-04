#include "Core.h"

/*
    Implementación de Light : IDevice
*/

Light::Light(uint8_t _pin, Data _data) : pin(_pin)
{
    data = _data;
}

void Light::init() {
    pinMode(pin, OUTPUT);
    status = Status::OFF;
}

void Light::activate() {
    digitalWrite(pin, HIGH);
    status = Status::ON;
}

void Light::desactivate() {
    digitalWrite(pin, LOW);
    status = Status::OFF;
}