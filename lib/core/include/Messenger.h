#pragma once
#include <Arduino.h>


enum MessageStatus: uint8_t {
    STATUS_SUCCESS = 0,
    STATUS_ERROR = 1,
};


struct Message {
    MessageStatus status;
    char component[11];
    char text[32];

    Message(MessageStatus _status, const char* _component, const char* _text): status(_status) {
        strncpy(component, _component, sizeof(component));
        strncpy(text, _text, sizeof(text));
    }
};


class MessageBuilder {
    public:
        static MessageBuilder& getInstance() {
            static MessageBuilder instance;
            return instance;
        } 

        void setDevice(const char* device) {
            strncpy(_device, device, sizeof(_device) - 1);
        }
    
        void build(
            char* out, size_t outSize,
            const Message& msg
        ) {
            const char* comp = (msg.component[0] != '\0') ? msg.component : _device;
            snprintf(out, outSize, "msg:%s:%d:%s:%s\n", _device, msg.status, comp, msg.text);
        }
        MessageBuilder(const MessageBuilder&) = delete;
        MessageBuilder& operator=(const MessageBuilder&) = delete;
    private:
        char _device[10];
        MessageBuilder() {_device[0] = '\0';}
};


class SerialCaller {
public:
  static SerialCaller& getInstance() {
    static SerialCaller instance;
    return instance;
  }

  void begin(long baudRate) {
    Serial.begin(baudRate);
  }

  void send(const Message& msg) {
    char out[72];
    MessageBuilder::getInstance().build(out, sizeof(out), msg);
    Serial.print(out);
  }

  SerialCaller(const SerialCaller&)            = delete;
  SerialCaller& operator=(const SerialCaller&) = delete;

private:
  SerialCaller() {}
};