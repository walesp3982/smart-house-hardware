#include <Arduino.h>
#include <Wire.h>
#include <I2C_protocol.h>

#ifndef I2C_SLAVE_ADDR
  #error "I2C_SLAVE_ADDR no definido en platformio.ini"
#endif

static volatile bool alarm_active = false;
static volatile uint8_t last_cmd = 0x00;

static I2CPacket response;


// Configuración de comandos 

void onReceived(int bytes) {
  if(bytes != PKT_SIZE) {
    while(Wire.available()) Wire.read();
    return;
  }

  I2CPacket pkt;
  pkt.node_id = Wire.read();
  pkt.cmd = Wire.read();
  pkt.data = Wire.read(); 
  pkt.checksum = Wire.read();

  if(!pkt_valid(pkt)) return;

  last_cmd = pkt.cmd;

  switch (pkt.cmd)
  {
  case CMD_SET:
    alarm_active = (pkt.data == 0x01);
    digitalWrite(LED_BUILTIN, alarm_active ? HIGH : LOW);
    break;
  case CMD_STATUS:
  case CMD_PING:
    break;
  default:
    break;
  }

  response.node_id = NODE_ID;
  response.cmd = CMD_ACK;
  response.data = alarm_active ? 0x01 : 0x00;
  response.checksum = pkt_checksum(response);
}

void onRequest() {
  uint8_t* buf = (uint8_t*)&response;
  Wire.write(buf, PKT_SIZE);
}


enum class STATUS : uint8_t {
  OFF = 0,
  ON = 1
};
class BuzzerActuator {
  private:
    STATUS status;
    uint8_t pin_buzzer;
  public:
    BuzzerActuator(uint8_t _pin_buzzer) {
      pin_buzzer = _pin_buzzer;
      status = STATUS::ON;
    }
    void init() {
      pinMode(pin_buzzer, OUTPUT);
      desactivate();
    }
    void activate() {
      digitalWrite(pin_buzzer, HIGH);
      status = STATUS::ON;
    }
    void desactivate() {
      digitalWrite(pin_buzzer, LOW);
      status = STATUS::OFF;
    }
};

class MoveSensor {
  private:
    uint8_t pin_sensor;
  public:
    MoveSensor(uint8_t _pin_sensor) {
      pin_sensor = _pin_sensor;
    }

    void init() {
      pinMode(pin_sensor, INPUT);
    }

    int get_value() {
      return digitalRead(pin_sensor);
    }
};


BuzzerActuator buzzer(8);
MoveSensor move_sensor(7);

String comando = "";

void setup() {
  buzzer.init();
  move_sensor.init();
  Serial.begin(115200);
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onReceive(onReceived);
  Wire.onRequest(onRequest);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.print(F("Slave listo en 0x"));
  Serial.println(I2C_SLAVE_ADDR, HEX);
}

void loop() {
  delay(10);
  noInterrupts();
  bool active = alarm_active;
  interrupts();
  if (active) {
    if (move_sensor.get_value() == HIGH) {
      buzzer.activate();
    } else if (move_sensor.get_value() == LOW) {
      buzzer.desactivate();
    }
  } else {
    buzzer.desactivate();
  }
}