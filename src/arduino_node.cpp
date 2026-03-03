#include <Wire.h>

void setup() {
    Wire.begin(); 
    int a = 5;
}

void loop() {
    Wire.beginTransmission(8);
    Wire.write("ON");
    Wire.endTransmission();
    delay(1000);
}