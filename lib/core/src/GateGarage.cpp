#include "Core.h"
#include "Messenger.h"
GateGarage::GateGarage(uint8_t _pin, int _rotation, Data _data) : Gate(_pin, _rotation, _data) {}

void GateGarage::activate() {
    Gate::motor.write(0);
    status = ON;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_SUCCESS, data.code, "Abierto correctamente")
    );
}

void GateGarage::desactivate() {
    Gate::motor.write(rotation);
    status = OFF;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_ERROR, data.code, "Cerrado correctamente")
    );
}
