#pragma once
#include <Arduino.h>

class Core {
    public:
        static void initSerial(long baud = 115200);
};