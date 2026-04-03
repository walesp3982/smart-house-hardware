#include "mqtt_handler.h"
#include "camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>   // ArduinoJson @ ^6

// Construidos en setup desde el UUID
static char _topic_set[80];     // /{uuid}/set     — recibe comandos
static char _topic_state[80];   // /{uuid}/state   — publica estado
static char _topic_stream[80];  // /{uuid}/stream_url

static const char* _user;
static const char* _pass;
static String      _stream_url;

static WiFiClient   wifiClient;
static PubSubClient mqtt(wifiClient);

// ── Publicar estado actual ──────────────────────────────────────────────────
void mqtt_publish_state(bool camera_active, const char* stream_url) {
    StaticJsonDocument<200> doc;
    doc["active"]     = camera_active;
    doc["stream_url"] = camera_active ? stream_url : "";
    doc["ip"]         = WiFi.localIP().toString();

    char buf[200];
    serializeJson(doc, buf);
    mqtt.publish(_topic_state, buf, true);   // retained=true → el broker guarda el último estado
}

// ── Callback de mensajes entrantes ─────────────────────────────────────────
static void on_message(char* topic, byte* payload, unsigned int len) {
    // Deserializar payload JSON
    StaticJsonDocument<200> doc;
    DeserializationError err = deserializeJson(doc, payload, len);
    if (err) {
        Serial.printf("[MQTT] JSON inválido: %s\n", err.c_str());
        return;
    }

    const char* action = doc["action"];
    if (!action) return;

    Serial.printf("[MQTT] Acción recibida: %s\n", action);

    // ── Prender cámara ──────────────────────────────────────────────────────
    if (strcmp(action, "camera_on") == 0) {
        camera_set_active(true);
        mqtt_publish_state(true, _stream_url.c_str());
    }

    // ── Apagar cámara ───────────────────────────────────────────────────────
    else if (strcmp(action, "camera_off") == 0) {
        camera_set_active(false);
        mqtt_publish_state(false, "");
    }

    // ── Toggle ──────────────────────────────────────────────────────────────
    else if (strcmp(action, "camera_toggle") == 0) {
        bool now = !camera_is_active();
        camera_set_active(now);
        mqtt_publish_state(now, _stream_url.c_str());
    }

    // ── Solicitar captura ───────────────────────────────────────────────────
    // La cámara no envía la foto por MQTT (demasiado grande).
    // Publica la URL del endpoint de captura para que el backend la descargue.
    else if (strcmp(action, "snapshot") == 0) {
        if (!camera_is_active()) {
            Serial.println("[MQTT] Snapshot ignorado — cámara apagada");
            return;
        }
        char snapshot_url[100];
        snprintf(snapshot_url, sizeof(snapshot_url),
                 "http://%s/snapshot", WiFi.localIP().toString().c_str());

        StaticJsonDocument<200> resp;
        resp["snapshot_url"] = snapshot_url;
        char buf[200];
        serializeJson(resp, buf);

        // Publica la URL en state para que el backend la descargue via HTTP
        mqtt.publish(_topic_state, buf, false);
        Serial.printf("[MQTT] Snapshot disponible en %s\n", snapshot_url);
    }
}

// ── Reconexión ──────────────────────────────────────────────────────────────
static void reconnect() {
    while (!mqtt.connected()) {
        Serial.print("[MQTT] Conectando...");
        // Client ID único por dispositivo = UUID
        if (mqtt.connect(_topic_set, _user, _pass)) {   // reusa el topic como client ID
            Serial.println(" OK");
            mqtt.subscribe(_topic_set);
            Serial.printf("[MQTT] Suscrito a %s\n", _topic_set);
            // Publicar estado inicial al reconectar
            mqtt_publish_state(camera_is_active(), _stream_url.c_str());
        } else {
            Serial.printf(" fallo rc=%d, reintentando en 3s\n", mqtt.state());
            delay(3000);
        }
    }
}

void mqtt_setup(const char* uuid, const char* user, const char* pass) {
    _user = user;
    _pass = pass;

    snprintf(_topic_set,    sizeof(_topic_set),    "/%s/set",        uuid);
    snprintf(_topic_state,  sizeof(_topic_state),  "/%s/state",      uuid);
    snprintf(_topic_stream, sizeof(_topic_stream), "/%s/stream_url", uuid);

    _stream_url = "http://" + WiFi.localIP().toString() + "/stream";

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(on_message);
    mqtt.setBufferSize(512);

    reconnect();
}

void mqtt_loop() {
    if (!mqtt.connected()) reconnect();
    mqtt.loop();
}