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
Light luz_sala(10, Data{"Luz Sala", "L1"});

void setup() {
  // Agregamos los dispositivos
  controller.add_device(&puertaPrincipal);
  controller.add_device(&garaje);
  controller.add_device(&dormitorio);
  controller.add_device(&luz_sala);
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