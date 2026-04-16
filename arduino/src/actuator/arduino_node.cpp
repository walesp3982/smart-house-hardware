//#include<Arduino.h>
//#include<Servo.h>
#include "Core.h"
#include "Messenger.h"
#include "Wire.h"

const int BUFFER_SIZE = 32;  // Reducido de 50 para ahorrar RAM
char inputBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;


const char* NAMING = "ard1";
DevicesController controller(NAMING);
// Creacion de dispositivos
// Declaración de bits para cada dispositivo
constexpr uint8_t BIT_PUERTA_PRINCIPAL = 0;
constexpr uint8_t BIT_COMPUERTA_GARAGE = 1;
constexpr uint8_t BIT_PUERTA_DORMITORIO = 2;
constexpr uint8_t BIT_LUZ_GARAGE = 3;
constexpr uint8_t BIT_LUZ_DORMITORIO = 4;
constexpr uint8_t BIT_LUZ_SALA = 5;
constexpr uint8_t BIT_LUZ_COCINA = 6;


Gate puertaPrincipal(3, 90, Data{"Puerta principal", "P1" }, BIT_PUERTA_PRINCIPAL);
GateGarage garaje(5, 90, Data{"Puerta Garaje", "P2"}, BIT_COMPUERTA_GARAGE);
Gate dormitorio(6, 90, Data{"Puerta Dormitorio", "P3"}, BIT_PUERTA_DORMITORIO);
Light luz_garage(8, Data{"Luz Garaje", "L1"}, BIT_LUZ_GARAGE);
Light luz_dormitorio(9, Data{"Luz Dormitorio", "L2"}, BIT_LUZ_DORMITORIO);
Light luz_sala(10, Data{"Luz sala", "L3"}, BIT_LUZ_SALA);
Light luz_cocina(11, Data{"Luz cocina", "L3"}, BIT_LUZ_COCINA);

#if MODE == 1
  static I2CPacket response;
  void onReceived(int bytes) {
    Serial.print("[I2C_RX] PKT recibido bytes=");
    Serial.println(bytes);
    
    if(bytes != PKT_SIZE) {
      Serial.print("[I2C_RX] ERROR: esperaba ");
      Serial.print(PKT_SIZE);
      Serial.print(" bytes, recibio ");
      Serial.println(bytes);
      while(Wire.available()) Wire.read();
      return;
    }

    I2CPacket pkt;
    pkt.node_id = Wire.read();
    pkt.cmd = Wire.read();
    pkt.data = Wire.read();
    pkt.checksum = Wire.read();

    Serial.print("[I2C_RX] node_id=");
    Serial.print(pkt.node_id);
    Serial.print(" cmd=0x");
    Serial.print(pkt.cmd, HEX);
    Serial.print(" data=0x");
    Serial.print(pkt.data, HEX);
    Serial.print(" checksum=0x");
    Serial.println(pkt.checksum, HEX);
    
    Serial.print("[I2C_RX] NODE_ID esperado=");
    Serial.println(NODE_ID);

    if(pkt.node_id != NODE_ID) {
      Serial.print("[I2C_RX] ERROR: node_id mismatch (recibido ");
      Serial.print(pkt.node_id);
      Serial.print(", esperado ");
      Serial.println(NODE_ID);
      return;
    }
    
    uint8_t expected_checksum = pkt_checksum(pkt);
    Serial.print("[I2C_RX] checksum recibido=0x");
    Serial.print(pkt.checksum, HEX);
    Serial.print(", calculado=0x");
    Serial.println(expected_checksum, HEX);
    
    if(!pkt_valid(pkt)) {
      Serial.println("[I2C_RX] ERROR: checksum invalido");
      return;
    }

    Serial.print("[I2C_RX] Paquete valido, ejecutando CMD=0x");
    Serial.println(pkt.cmd, HEX);

    switch (pkt.cmd)
    {
    case CMD_SET:
      Serial.print("[I2C_RX] Ejecutando CMD_SET con data=0x");
      Serial.println(pkt.data, HEX);
      controller.execute_i2c(pkt);
      break;
    case CMD_STATUS:
      Serial.println("[I2C_RX] CMD_STATUS");
      break;
    case CMD_PING:
      Serial.println("[I2C_RX] CMD_PING");
      break;
    default:
      Serial.print("[I2C_RX] CMD desconocido: 0x");
      Serial.println(pkt.cmd, HEX);
      break;
    }

    response.node_id = NODE_ID;
    response.cmd = CMD_ACK;
    response.data = controller.get_status_devices_i2c();
    response.checksum = pkt_checksum(response);
    
    Serial.print("[I2C_TX_ACK] Enviando ACK con data=0x");
    Serial.print(response.data, HEX);
    Serial.print(" checksum=0x");
    Serial.println(response.checksum, HEX);
  }

  void onRequest() {
    uint8_t* buf = (uint8_t*)&response;
    Wire.write(buf, PKT_SIZE);
    Serial.println("[I2C_TX_REQ] onRequest enviado ACK");
  }
#endif

void setup() {
  // Setting Serialización
  MessageBuilder::getInstance().setDevice(NAMING);
  SerialCaller::getInstance().begin(9600);

  // Agregamos los dispositivos
  controller.add_device(&puertaPrincipal);
  controller.add_device(&garaje);
  controller.add_device(&dormitorio);
  controller.add_device(&luz_garage);
  controller.add_device(&luz_dormitorio);
  controller.add_device(&luz_sala);
  controller.add_device(&luz_cocina);
  
  // Configuración de pines
  controller.init();

  pinMode(LED_BUILTIN, OUTPUT);

  #if MODE == 1
    Wire.begin(I2C_SLAVE_ADDR);
    Wire.onReceive(onReceived);
    Wire.onRequest(onRequest);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("[SETUP] I2C SLAVE MODE ACTIVO - Addr=0x");
    Serial.print(I2C_SLAVE_ADDR, HEX);
    Serial.print(" NodeID=");
    Serial.println(NODE_ID);
    Serial.print("SIZE:");
    Serial.println(controller.get_size());
  #endif 
}

void loop() {
  #if MODE == 0
    if(SerialCaller::available()) {
      char c = SerialCaller::get_char();

      if(c == '\r') return;
      if(c == '\n') {
        inputBuffer[bufferIndex] = '\0';

        Command cmd;

        if(parseCommand(inputBuffer, cmd)) {
          controller.execute(cmd);
        }

        bufferIndex = 0;
      }
      else {
        if(bufferIndex < BUFFER_SIZE - 1) {
          inputBuffer[bufferIndex++] = c;
        }
      }
    }
    #endif
}