#pragma once
#include <Arduino.h>
#include <Core.h>

class SensorTemperature {
private:
  int pin;
public:
  SensorTemperature(int _pin) : pin(_pin) {}
  void init() {
    pinMode(pin, INPUT);
  }
  int8_t get_status() {
    int value = analogRead(pin);
    float voltaje = value * (5.0 / 1023.0);
    float temperatura = voltaje * 100;
    if (temperatura > INT8_MAX) temperatura = INT8_MAX;
    if (temperatura < INT8_MIN) temperatura = INT8_MIN;
    return static_cast<int8_t>(temperatura);
  }
};

class MoveSensor {
  private:
    uint8_t pin_sensor;
  public:
    MoveSensor(uint8_t _pin_sensor) {
      pin_sensor = _pin_sensor;
    }

    void init() {
      pinMode(pin_sensor, INPUT);
    }

    int get_value() {
      return digitalRead(pin_sensor);
    }
};
