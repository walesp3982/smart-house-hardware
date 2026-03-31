#include "Core.h"
#include "Messenger.h"
GateGarage::GateGarage(uint8_t _pin, int _rotation, Data _data, uint8_t _i2c_command) : Gate(_pin, _rotation, _data, _i2c_command) {}

void GateGarage::activate() {
    Gate::motor.write(0);
    status = Status::ON;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_SUCCESS, data.code, "Abierto correctamente")
    );
}

void GateGarage::desactivate() {
    Gate::motor.write(rotation);
    status = Status::OFF;
    SerialCaller::getInstance().send(
        Message(MessageStatus::STATUS_ERROR, data.code, "Cerrado correctamente")
    );
}
