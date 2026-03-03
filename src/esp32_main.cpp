#include <Wire.h>
#include <Arduino.h>
void receiveEvent(int bytes) {
    while (Wire.available()) {
        char c = Wire.read();
        Serial.print(c);
    }
}

void setup() {
    Wire.begin(8);
    Wire.onReceive(receiveEvent);
    Serial.begin(9600);
}

void loop() {}