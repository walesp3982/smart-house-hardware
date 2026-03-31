#pragma once

#include <Arduino.h>


enum class State: uint8_t {
    OFF = 0,
    ON = 1
};

class Device {
    public:
        virtual void publish() = 0;
        virtual void subscribe() = 0;
    private:
        char uuid4[40];    
        State state;    
};



