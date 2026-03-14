
#pragma once

enum Command {
    ON, OFF
};

enum TypeDevice {
    ACTUATOR, SENSOR
};

class Device {
    protected:
        char controller[11];
        char name[11];
        TypeDevice type;
    public:
        Device(const char* _controller, const char* _name );
        void execute(const Command &command, char* message, size_t size_message);
};
