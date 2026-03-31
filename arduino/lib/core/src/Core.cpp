#include "Core.h"
#include <string.h>
#include "Messenger.h"
/*
    Helpers para Comandos:
    Ignora cualquier input que comienze con "msg:..."
    Construye un Command structure
*/
bool parseCommand(char *input, Command &cmd)
{

    // 🔴 Ignorar mensajes que comiencen con "msg:"
    if (strncmp(input, "msg:", 4) == 0)
    {
        return false;
    }

    char *token;

    token = strtok(input, ":");
    if (!token)
        return false;
    strncpy(cmd.house, token, sizeof(cmd.house) - 1);
    cmd.house[sizeof(cmd.house) - 1] = '\0';

    token = strtok(NULL, ":");
    if (!token)
        return false;
    strncpy(cmd.device, token, sizeof(cmd.device) - 1);
    cmd.device[sizeof(cmd.device) - 1] = '\0';

    token = strtok(NULL, ":");
    if (!token)
        return false;
    strncpy(cmd.order, token, sizeof(cmd.order) - 1);
    cmd.order[sizeof(cmd.order) - 1] = '\0';

    return true;
}

/*
    Implementación de la clase IDevice
*/

Data IDevice::get_data()
{
    return data;
}

void IDevice::execute_command(char* order) {
    if(strcmp(order, "off") == 0) {
        desactivate();
    } else if(strcmp(order, "on") == 0){ 
        activate();
    }
}

Status IDevice::get_status() {
    return status;
}

uint8_t IDevice::get_bit_i2c() {
    return bit_data_i2c;
}


/*
    Implementación de la clase DevicesController
*/

DevicesController::DevicesController(const char *name)
{
    strncpy(_name, name, sizeof(_name) - 1);
    _name[sizeof(_name) - 1] = '\0';
}

void DevicesController::add_device(IDevice *device)
{
    if (_size >= MAX_DEVICES) {
        SerialCaller::getInstance().send(
            Message(MessageStatus::STATUS_ERROR, "", "WARNING Device LIMIT ERROR")
        );
        return; 
    }
    _devices[_size++] = device;
}

void DevicesController::init()
{
    for (int i = 0; i < _size; i++)
    {
        IDevice *device = _devices[i];
        device->init();
    }
}

void DevicesController::execute(Command &command)
{
    // Name no encontrado
    if (strcmp(_name, command.house) != 0)
    {
        return;
    }
    if (strcmp("info", command.device) == 0)
    {
        if (strcmp("name", command.order) == 0)
        {
            Serial.print("msg:ard:name:");
            Serial.println(_name);
        }
        if (strcmp("devices", command.order) == 0)
        {
            // TODO: list devices format "nickname-type:nickname2-type2"
        }
        return;
    }
    for (int i = 0; i < _size; i++)
    {
        IDevice *device = get_device(i);
        if (!device)
            break;

        if (strcmp(device->get_data().code, command.device) == 0)
        {
            device->execute_command(command.order);
        }
    }
}

IDevice *DevicesController::get_device(int id)
{
    if (id < 0 || id > _size)
        return NULL;
    return _devices[id];
}

void DevicesController::execute_i2c(I2CPacket packet) {
    uint8_t devices = packet.data;

    for(int i = 0; i < _size; i++) {
        IDevice* device = get_device(i);
        if (!device) break;

        uint8_t bit_device = device->get_bit_i2c();
        ((devices >> bit_device)&1) ? device->activate(): device->desactivate();
    }
}


uint8_t DevicesController::get_status_devices() {
    uint8_t data = 0b00000000;
    for(int i = 0; i < _size; i++) {
        IDevice* device = get_device(i);
        if (!device) break;
        if (device->get_status() == Status::ON) {
            data |= (1 << device->get_bit_i2c());
        }
    }
    return data;
}