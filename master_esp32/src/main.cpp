#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
// ─────────────────────────────────────────
//  PINES — AI Thinker ESP32-CAM
// ─────────────────────────────────────────
#include "PinsConfig.h"

WebServer server(80);

bool flashOn  = false;
bool ledRojoOn = false;

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;  // 0-63, menor = mejor calidad
  config.fb_count     = 2;
  config.frame_size   = FRAMESIZE_VGA;  // 640x480

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  Serial.println("Camera OK");
  return true;
}

// ─────────────────────────────────────────
//  HANDLERS HTTP
// ─────────────────────────────────────────

// GET /  → página web


// GET /flash/on  y  /flash/off
void handleFlashOn() {
  flashOn = true;
  digitalWrite(LED_FLASH, HIGH);
  server.send(200, "application/json", "{\"flash\":true}");
}
void handleFlashOff() {
  flashOn = false;
  digitalWrite(LED_FLASH, LOW);
  server.send(200, "application/json", "{\"flash\":false}");
}

// GET /led/on  y  /led/off
void handleLedOn() {
  ledRojoOn = true;
  digitalWrite(LED_ROJO, LOW);   // activo en LOW
  server.send(200, "application/json", "{\"led\":true}");
}
void handleLedOff() {
  ledRojoOn = false;
  digitalWrite(LED_ROJO, HIGH);
  server.send(200, "application/json", "{\"led\":false}");
}

// GET /status → estado actual
void handleStatus() {
  String json = "{\"flash\":" + String(flashOn ? "true" : "false") +
                ",\"led\":"   + String(ledRojoOn ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

// GET /capture → JPEG snapshot
void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

// GET /stream → MJPEG stream
void handleStream() {
  WiFiClient client = server.client();

  // Cabecera multipart
  String header =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(header);

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { Serial.println("Frame fail"); break; }

    String part =
      "--frame\r\n"
      "Content-Type: image/jpeg\r\n"
      "Content-Length: " + String(fb->len) + "\r\n\r\n";
    client.print(part);
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);
    delay(30);  // ~30 fps máx
  }
}

// ─────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32-CAM START ===");

  // LEDs
  pinMode(LED_FLASH, OUTPUT);
  pinMode(LED_ROJO,  OUTPUT);
  digitalWrite(LED_FLASH, LOW);
  digitalWrite(LED_ROJO,  HIGH);  // apagado (activo LOW)

  // Cámara
  if (!initCamera()) {
    Serial.println("ERROR: Camera failed. Reiniciando en 5s...");
    delay(5000);
    ESP.restart();
  }

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Parpadeo LED rojo mientras conecta
    digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));
  }
  digitalWrite(LED_ROJO, HIGH); // apagar al conectar
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Rutas HTTP
  server.on("/flash/on",   HTTP_GET, handleFlashOn);
  server.on("/flash/off",  HTTP_GET, handleFlashOff);
  server.on("/led/on",     HTTP_GET, handleLedOn);
  server.on("/led/off",    HTTP_GET, handleLedOff);
  server.on("/status",     HTTP_GET, handleStatus);
  server.on("/capture",    HTTP_GET, handleCapture);
  server.on("/stream",     HTTP_GET, handleStream);

  server.begin();
  Serial.println("Servidor HTTP listo");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("Rutas disponibles:");
  Serial.println("  GET /           → Panel web");
  Serial.println("  GET /stream     → MJPEG stream");
  Serial.println("  GET /capture    → JPEG snapshot");
  Serial.println("  GET /flash/on   → Flash encender");
  Serial.println("  GET /flash/off  → Flash apagar");
  Serial.println("  GET /led/on     → LED rojo encender");
  Serial.println("  GET /led/off    → LED rojo apagar");
  Serial.println("  GET /status     → Estado JSON");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━");
}

// ─────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────
void loop() {
  server.handleClient();
}