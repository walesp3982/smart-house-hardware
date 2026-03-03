#include "Core.h"

void Core::initSerial(long baud) {
    Serial.begin(baud);
}