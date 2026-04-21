#include <Arduino.h>
#include <WiFi.h>
#include "esp_http_server.h"
#include "camera.h"
#include "mqtt_handler.h"
#include "generated_devices.h"
#include <Preferences.h>

// Provistos por build_flags en platformio.ini
#ifndef MQTT_USER
  #define MQTT_USER ""
#endif
#ifndef MQTT_PASSWORD
  #define MQTT_PASSWORD
 ""
#endif
#ifndef UUID_CAMERA
  #define UUID_CAMERA "00000000-0000-0000-0000-000000000000"
#endif


static httpd_handle_t server = nullptr;
char CHIP_ID[30] = "espcam-abc123";
Preferences preferences;
// ── Handler: MJPEG stream ───────────────────────────────────────────────────
static esp_err_t stream_handler(httpd_req_t* req) {
    if (!camera_is_active()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera off");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char part_buf[64];
    while (true) {
        camera_fb_t* fb = camera_capture();
        if (!fb) continue;

        snprintf(part_buf, sizeof(part_buf),
                 "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                 fb->len);

        esp_err_t res = httpd_resp_send_chunk(req, part_buf, strlen(part_buf));
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb);

        if (res != ESP_OK) break;   // cliente desconectado
    }
    return ESP_OK;
}

// ── Handler: snapshot JPEG ──────────────────────────────────────────────────
static esp_err_t snapshot_handler(httpd_req_t* req) {
    if (!camera_is_active()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera off");
        return ESP_FAIL;
    }

    camera_fb_t* fb = camera_capture();
    if (!fb) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Capture failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=snapshot.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, (const char*)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    return ESP_OK;
}

// ── Iniciar servidor HTTP ───────────────────────────────────────────────────
static void start_http_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port    = 80;

    if (httpd_start(&server, &config) != ESP_OK) {
        Serial.println("[HTTP] Error al iniciar servidor");
        return;
    }

    httpd_uri_t stream_uri = {
        .uri      = "/stream",
        .method   = HTTP_GET,
        .handler  = stream_handler,
        .user_ctx = nullptr
    };
    httpd_uri_t snapshot_uri = {
        .uri      = "/snapshot",
        .method   = HTTP_GET,
        .handler  = snapshot_handler,
        .user_ctx = nullptr
    };

    httpd_register_uri_handler(server, &stream_uri);
    httpd_register_uri_handler(server, &snapshot_uri);

    Serial.printf("[HTTP] Stream:   http://%s/stream\n",   WiFi.localIP().toString().c_str());
    Serial.printf("[HTTP] Snapshot: http://%s/snapshot\n", WiFi.localIP().toString().c_str());
}

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1000);
    // WiFi
    Serial.print("[WiFi] Conectando");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());

    // Cámara — arranca activa por defecto
    camera_init();

    preferences.begin("my-app", false);
    bool camera_state = preferences.getBool("camera_state", true);  // por defecto on
    camera_set_active(camera_state);
    Serial.print("Estado persistente - Cámara: ");
    Serial.println(camera_state ? "Encendida" : "Apagada");
    preferences.end();

    // Servidor HTTP
    start_http_server();

    // MQTT
    mqtt_setup(UUID_CAMERA, MQTT_USER, MQTT_PASSWORD, CHIP_ID);
}


void save_status_camera() {
    preferences.begin("my-app", false);
    bool camera_state = camera_is_active();
    preferences.putBool("camera_state", camera_state);
    Serial.print("Estado persistente - Cámara: ");
    Serial.println(camera_state ? "Encendida" : "Apagada");
    preferences.end();
}

static long last_persistence_update = 0;
// ── Loop ────────────────────────────────────────────────────────────────────


void loop() {
    unsigned long now = millis();

    mqtt_loop(CHIP_ID);
    delay(10);

    // Guardamos el estado de la cámara cada 2 segundos
    // (o cuando cambie) para tener persistencia en caso de reinicio o fallo)
    if (now - last_persistence_update > 2000) {   // cada 2 segundos
        save_status_camera();
        last_persistence_update = now;
    }
}