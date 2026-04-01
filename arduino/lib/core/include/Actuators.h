#pragma once 
#include <Arduino.h>
#include <Core.h>

class VentilatorActuator {
private:
  int pin;
  Status status;
public:
  VentilatorActuator(int _pin) : pin(_pin), status(Status::OFF) {}
  void init() {
    pinMode(pin, OUTPUT);
    desactivate();
  }
  void activate() {
    digitalWrite(pin, HIGH);
    status = Status::ON;
  }
  void desactivate() {
    digitalWrite(pin, LOW);
    status = Status::OFF;
  }
  Status get_status() const {
    return status;
  }
};

class BuzzerActuator {
  private:
    Status status;
    uint8_t pin_buzzer;
  public:
    BuzzerActuator(uint8_t _pin_buzzer) {
      pin_buzzer = _pin_buzzer;
      status = Status::ON;
    }
    void init() {
      pinMode(pin_buzzer, OUTPUT);
      desactivate();
    }
    void activate() {
      digitalWrite(pin_buzzer, HIGH);
      status = Status::ON;
    }
    void desactivate() {
      digitalWrite(pin_buzzer, LOW);
      status = Status::OFF;
    }
};