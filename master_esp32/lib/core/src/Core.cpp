#include<Core.h>
#include<Arduino.h>

void safestrncpy(char *cpy, const char *original, size_t size)
{
    strncpy(cpy, original, size);
    cpy[size - 1] = '\0';
}


Device::Device(const char* _controller, const char* _name) {
    safestrncpy(controller, _controller, sizeof(controller));
    safestrncpy(name, _name, sizeof(name));
}

void Device::execute(const Command &command, char* message, size_t size_message) {
    char order[4];
    if (command == ON) {
        safestrncpy(order, "ON", sizeof(order));
    } else if (command == OFF) {
        safestrncpy(order, "OFF", sizeof(order));
    }
    snprintf(message, size_message, "%s:%s:%s\n", controller, name, order);
}