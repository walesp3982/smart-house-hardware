#include<Arduino.h>
#include<Servo.h>

Servo puertaPrincipal;
Servo garaje;
Servo dormitorio;

void setup() {
  Serial.begin(9600);

  puertaPrincipal.attach(3);
  garaje.attach(5);
  dormitorio.attach(6);
  Serial.print("Hola a todo el mundo");
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');

    if (comando == "PUERTA") {
      puertaPrincipal.write(90);
    }

    if (comando == "GARAJE") {
      garaje.write(90);
    }

    if (comando == "DORMITORIO") {
      dormitorio.write(90);
    }

    if (comando == "LUZ_GARAJE") {
      digitalWrite(8, HIGH);
    }

    if (comando == "LUZ_SALA_COCINA") {
      digitalWrite(10, HIGH);
      digitalWrite(11, HIGH);
    }
  }
}