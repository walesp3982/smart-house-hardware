#include <Arduino.h>

int sensorMovimiento = 7;   // Pin del FC-51
int buzzer = 8;             // Pin del buzzer

bool alarmaActiva = false;  // Estado de la alarma
String comando = "";

void setup() {
  Serial.begin(9600);
  pinMode(sensorMovimiento, INPUT);
  pinMode(buzzer, OUTPUT);

  Serial.println("Sistema de alarma listo");
}

void loop() {

  // Leer comandos desde Python
  if (Serial.available()) {
    comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "ON") {
      alarmaActiva = true;
      Serial.println("Alarma ACTIVADA");
    } 
    else if (comando == "OFF") {
      alarmaActiva = false;
      digitalWrite(buzzer, LOW);
      Serial.println("Alarma DESACTIVADA");
    }
  }

  // Funcionamiento de la alarma
  if (alarmaActiva) {
    int movimiento = digitalRead(sensorMovimiento);

    if (movimiento == HIGH) {
      Serial.println("INTRUSO DETECTADO");
      digitalWrite(buzzer, HIGH);
    } else {
      digitalWrite(buzzer, LOW);
    }
  }

  delay(500);
}