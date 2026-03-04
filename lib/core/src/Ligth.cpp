#include "Core.h"
#include "Messenger.h"
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
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_SUCCESS, data.code, "Luz prendida")
    );
}

void Light::desactivate() {
    digitalWrite(pin, LOW);
    status = Status::OFF;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_SUCCESS, data.code, "Luz apagada")
    );
}