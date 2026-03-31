#include <Arduino.h>
int sensorPin = A0;
int ventilador = 8;

float temperatura;
String comando = "";
bool modoAutomatico = true;

void setup() {
  Serial.begin(9600);
  pinMode(ventilador, OUTPUT);
}

void loop() {
  int valor = analogRead(sensorPin);
  float voltaje = valor * (5.0 / 1023.0);
  temperatura = voltaje * 100;

  Serial.print("TEMP:");
  Serial.println(temperatura);

  if (Serial.available()) {
    comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "ON") {
      modoAutomatico = false;
      digitalWrite(ventilador, HIGH);
    } 
    else if (comando == "OFF") {
      modoAutomatico = false;
      digitalWrite(ventilador, LOW);
    } 
    else if (comando == "AUTO") {
      modoAutomatico = true;
    }
  }

  if (modoAutomatico) {
    if (temperatura > 30) {
      digitalWrite(ventilador, HIGH);
    } else if (temperatura < 28) {
      digitalWrite(ventilador, LOW);
    }
  }

  delay(1000);
}