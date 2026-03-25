#include<Arduino.h>
int sensorMovimiento = 7;
int buzzer = 8;

bool sistemaActivo = false;
String comando = "";

void setup() {
  Serial.begin(9600);
  pinMode(sensorMovimiento, INPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {

  if (Serial.available()) {
    comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "ACTIVAR") {
      sistemaActivo = true;
    } 
    else if (comando == "DESACTIVAR") {
      sistemaActivo = false;
      digitalWrite(buzzer, LOW);
    }
  }

  if (sistemaActivo) {
    int estado = digitalRead(sensorMovimiento);

    if (estado == HIGH) {
      digitalWrite(buzzer, HIGH);
      Serial.println("MOVIMIENTO DETECTADO");
    } else {
      digitalWrite(buzzer, LOW);
      Serial.println("SIN MOVIMIENTO");
    }
  } else {
    Serial.println("SISTEMA APAGADO");
  }

  delay(500);
}