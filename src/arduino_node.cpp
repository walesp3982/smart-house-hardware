#include<Arduino.h>
#include<Servo.h>
#include "Core.h"
#include "Messenger.h"
const int BUFFER_SIZE = 50;
char inputBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;


const char* NAMING = "ard1";
DevicesController controller(NAMING);
// Creacion de dispositivos
Gate puertaPrincipal(3, 90, Data{"Puerta principal", "P1" });
Gate garaje(5, 90, Data{"Puerta Garaje", "P2"});
Gate dormitorio(6, 90, Data{"Puerta Dormitorio", "P3"});


Light luz_garage(8, Data{"Luz Garaje", "L1"});
Light luz_dormitorio(9, Data{"Luz Dormitorio", "L2"});
Light luz_sala(10, Data{"Luz sala", "L3"});
Light luz_cocina(11, Data{"Luz cocina", "L3"});


void setup() {
  // Agregamos los dispositivos
  controller.add_device(&puertaPrincipal);
  controller.add_device(&garaje);
  controller.add_device(&dormitorio);
  controller.add_device(&luz_sala);
  controller.add_device(&luz_garage);
  controller.add_device(&luz_dormitorio);
  controller.add_device(&luz_cocina);
  // Inicializamos los dispositivos (Pines)
  // Setting Serialización
  MessageBuilder::getInstance().setDevice(NAMING);
  SerialCaller::getInstance().begin(9600);

  // Configuración de pines
  controller.init();
}

void loop() {

  if(Serial.available()) {
    char c = Serial.read();

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