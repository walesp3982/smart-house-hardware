#include <Arduino.h>
#include <I2C_protocol.h>
#include <Wire.h>
// ventilador puede prender y apagar por parte del usuario
// ventilador se puede configurar su temperatura?? Si
// modo automático

enum class Status : uint8_t
{
  OFF = 0,
  ON = 1
};

class SensorTemperature
{
private:
  int pin;
  int value; // Valor en celcius
public:
  SensorTemperature(int _pin) : pin(_pin) {}

  void init()
  {
    pinMode(pin, INPUT);
  }
  int8_t get_status()
  {
    int value = analogRead(pin);
    float voltaje = value * (5.0 / 1023.0);
    float temperatura = voltaje * 100;

    if (temperatura > INT8_MAX)
      temperatura = INT8_MAX;
    else if (temperatura < INT8_MIN)
      temperatura = INT8_MIN;
    return (int8_t)temperatura;
  }
};

class VentilatorActuator
{
private:
  int pin;
  Status status;

public:
  VentilatorActuator(int _pin) : pin(_pin), status(Status::OFF)
  {
  }
  void init()
  {
    pinMode(pin, OUTPUT);
  }

  void activate()
  {
    digitalWrite(pin, HIGH);
    status = Status::ON;
  }

  void desactivate()
  {
    digitalWrite(pin, LOW);
    status = Status::OFF;
  }
};

#if MODE == 1
static I2CPacket pkt;

static I2CPacket response;
bool unpack_flag(uint8_t byte) {
  return (byte >> 7) & 0x01;
}

uint8_t unpack_value(uint8_t byte) {
  return byte & 0x7F;
}

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

  switch (pkt.cmd)
  {
  case CMD_SET:
    enabled_automatic = unpack_flag(pkt.data);
    temperature_limit = unpack_value(pkt.data);
    // data = 00000000b
    // 2^6 = 64
    // 
    //  = 0
    break;
  
  default:
    break;
  }

  response.node_id = NODE_ID;
  response.data = (uint8_t)  value_temperature;
  response.cmd = CMD_ACK;
  response.checksum = pkt_checksum(response);
  


}

void onRequest() {
  uint8_t* buf = (uint8_t*)&response;
  Wire.write(buf,PKT_SIZE);
}
#endif

static volatile int temperature_limit = 50;
static volatile bool enabled_automatic = true;
static int8_t value_temperature = 0;
SensorTemperature temperature(A0);
VentilatorActuator ventilador(8);

String comando = "";

void setup()
{
  Serial.begin(9600);
  temperature.init();
  ventilador.init();

  pinMode(LED_BUILTIN, OUTPUT);
  #if MODE == 1
    Wire.begin(I2C_SLAVE_ADDR);
    Wire.onReceive(onReceived);
    Wire.onRequest(onRequest);

    digitalWrite(LED_BUILTIN, HIGH);
  #else 
    digitalWrite(LED_BUILTIN, LOW);
  #endif
}

void loop()
{
  value_temperature = temperature.get_status();

#if MODE == 0
  Serial.print("TEMP:");
  Serial.println(value_temperature);

  if (Serial.available())
  {
    comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "ON")
    {
      enabled_automatic = false;
      ventilador.activate();
    }
    else if (comando == "OFF")
    {
      enabled_automatic = false;
      ventilador.desactivate();
    }
    else if (comando == "AUTO")
    {
      enabled_automatic = true;
    }
  }
#endif
  /** 
   * Modo automático
   */
  if (enabled_automatic)
  {
    if (value_temperature > temperature_limit)
    {
      ventilador.activate();
    }
    else if (value_temperature < temperature_limit)
    {
      ventilador.desactivate();
    }
  }

  delay(1000);
}