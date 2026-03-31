//#include<Arduino.h>
//#include<Servo.h>
#include "Core.h"
#include "Messenger.h"
const int BUFFER_SIZE = 50;
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
}

void loop() {

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
}