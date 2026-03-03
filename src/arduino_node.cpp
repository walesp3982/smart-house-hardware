#include <Wire.h>

void setup() {
    Wire.begin(); 
}

void loop() {
    Wire.beginTransmission(8);
    Wire.write("ON");
    Wire.endTransmission();
    delay(1000);
}