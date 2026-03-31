#include <Arduino.h>
#include <I2C_protocol.h>
#include <Wire.h>

enum class Status : uint8_t {
  OFF = 0,
  ON = 1
};

class SensorTemperature {
private:
  int pin;
public:
  SensorTemperature(int _pin) : pin(_pin) {}
  void init() {
    pinMode(pin, INPUT);
  }
  int8_t get_status() {
    int value = analogRead(pin);
    float voltaje = value * (5.0 / 1023.0);
    float temperatura = voltaje * 100;
    if (temperatura > INT8_MAX) temperatura = INT8_MAX;
    if (temperatura < INT8_MIN) temperatura = INT8_MIN;
    return static_cast<int8_t>(temperatura);
  }
};

class VentilatorActuator {
private:
  int pin;
  Status status;
public:
  VentilatorActuator(int _pin) : pin(_pin), status(Status::OFF) {}
  void init() {
    pinMode(pin, OUTPUT);
    desactivate();
  }
  void activate() {
    digitalWrite(pin, HIGH);
    status = Status::ON;
  }
  void desactivate() {
    digitalWrite(pin, LOW);
    status = Status::OFF;
  }
  Status get_status() const {
    return status;
  }
};

static volatile uint8_t temperature_limit = 50;
static volatile bool enabled_automatic = true;
static volatile int8_t value_temperature = 0;
static volatile Status fan_state = Status::OFF;
SensorTemperature temperature(A0);
VentilatorActuator ventilador(8);

#if MODE == 1
static I2CPacket response;

bool unpack_flag(uint8_t byte) {
  return (byte >> 7) & 0x01;
}

uint8_t unpack_limit(uint8_t byte) {
  return byte & 0x3F;
}

void apply_temperature_command(uint8_t data) {
  enabled_automatic = unpack_flag(data);
  temperature_limit = unpack_limit(data);
  bool request_on = (data >> 6) & 0x01;

  if (!enabled_automatic) {
    if (request_on) {
      ventilador.activate();
      fan_state = Status::ON;
    } else {
      ventilador.desactivate();
      fan_state = Status::OFF;
    }
  }
}

void update_fan() {
  if (enabled_automatic) {
    if (value_temperature > temperature_limit) {
      ventilador.activate();
      fan_state = Status::ON;
    } else {
      ventilador.desactivate();
      fan_state = Status::OFF;
    }
  }
}

void setResponse() {
  response.node_id = NODE_ID;
  response.cmd = CMD_ACK;
  uint8_t state_bit = (fan_state == Status::ON) ? 0x40 : 0x00;
  uint8_t auto_bit = enabled_automatic ? 0x80 : 0x00;
  response.data = auto_bit | state_bit | (temperature_limit & 0x3F);
  response.checksum = pkt_checksum(response);
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
  if(pkt.node_id != NODE_ID) return;

  switch (pkt.cmd) {
    case CMD_SET:
      apply_temperature_command(pkt.data);
      break;
    case CMD_STATUS:
    case CMD_PING:
      break;
    default:
      break;
  }

  setResponse();
}

void onRequest() {
  setResponse();
  uint8_t* buf = reinterpret_cast<uint8_t*>(&response);
  Wire.write(buf, PKT_SIZE);
}
#else
void update_fan() {
  if (enabled_automatic) {
    if (value_temperature > temperature_limit) {
      ventilador.activate();
      fan_state = Status::ON;
    } else {
      ventilador.desactivate();
      fan_state = Status::OFF;
    }
  }
}
#endif

void setup() {
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

void loop() {
  value_temperature = temperature.get_status();
  update_fan();

#if MODE == 0
  Serial.print("TEMP:");
  Serial.println(value_temperature);

  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando == "ON") {
      enabled_automatic = false;
      ventilador.activate();
      fan_state = Status::ON;
    } else if (comando == "OFF") {
      enabled_automatic = false;
      ventilador.desactivate();
      fan_state = Status::OFF;
    } else if (comando == "AUTO") {
      enabled_automatic = true;
    }
  }
#endif
  delay(1000);
}
