#pragma once
#include <Arduino.h>
#include <Servo.h>
#include <I2C_protocol.h>
const int MAX_DEVICES = 10;

enum class Status: uint8_t {
    OFF=0,
    ON=1
};

struct Command {
    char house[11];
    char device[11];
    char order[11];
};

bool parseCommand(char* input, Command& cmd);

struct Data {
    char name[20];
    char code[5];
};

class IDevice {
    public:
        virtual void init() = 0;
        virtual void activate() = 0;
        virtual void desactivate() = 0;
        void execute_command(char * order);
        virtual ~IDevice() {}
        Data get_data();
        Status get_status();
        uint8_t get_bit_i2c();
    protected:
        Status status;
        Data data;
        uint8_t bit_data_i2c;
};

class DevicesController {
    public:
        DevicesController(const char* name) ;
        void add_device(IDevice* device);
        void init();
        void execute(Command &command);
        IDevice* get_device(int pos);
        void execute_i2c(I2CPacket packet);
        uint8_t get_status_devices_i2c();
    private:
        IDevice* _devices[MAX_DEVICES];
        uint8_t _size;
        char _name[11];
};



class Gate: public IDevice {
    public:
        Gate(uint8_t _pin, int _rotation, Data _data, uint8_t _bit_data_i2c);
        void activate() override;
        void init() override;
        void desactivate() override;
    protected:
        Servo motor;
        uint8_t pin;
        int rotation;
};

class GateGarage: public Gate {
    public:
        GateGarage(uint8_t _pin, int _rotation, Data _data, uint8_t _bit_data_i2c);
        void activate() override;
        void desactivate() override;
    };

class Light: public IDevice {
    public:
        Light(uint8_t _pin, Data _data, uint8_t _bit_data_i2c);
        void init() override;
        void activate() override;
        void desactivate() override;

    private:
        uint8_t pin;

};
// class SensorTemperature: public IDevice {
//     public:
//         SensorTemperature();
//         void activate() override;
//         void desactivate() override;
//         Status get_status() override;
// };

// class Window: public Gate {
//     public:
//         Window(uint8_t _pin, int _rotation);
// };

// class Garage: public Gate {
//     public:
//         Garage(uint8_t _pin, int _rotation);
// };

// class Camara: public IDevice {
//     public:
//         Camara();
//         void activate() override;
//         void desactivate() override;
//         Status get_status() override;
// };

// class Door: public IDevice {
//     public:
//         Door();
//         void activate() override;
//         void desactivate() override;
//         Status get_status() override;
// };

