#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "i2c_protocol.h"
#include "generated_devices.h"
#include "builder.h"
#include "actuators.h"
#include "controller.h"
#include "movement.h"
#include "thermostat.h"
#include <Preferences.h>

Actuator *door_principal;

Actuator *door_garage;
Actuator *door_dormitorio;
Actuator *luz_garage;

Actuator *luz_dormitorio;

Actuator *luz_sala;
Actuator *luz_cocina;

constexpr uint8_t ADDR_ALARM = 0x08;
constexpr uint8_t ADDR_ACTUATORS = 0x09;
constexpr uint8_t ADDR_TEMPERATURE = 0x0A;

constexpr uint8_t NODE_ID_ALARM = 1;
constexpr uint8_t NODE_ID_ACTUATORS = 2;
constexpr uint8_t NODE_ID_TEMPERATURE = 3;
ActuatorsController *actuator_controller;
TemperatureController *temperature_controller;
MoveController *move_controller;
DevicesController *devices_controller;

WiFiClient wifiClient;
PubSubClient *mqtt;

static char mqtt_client_id[32] = {0};
static unsigned long last_status_poll = 0;

String namespace_actuators = "actuators";
String namespace_door_garage = "P1";
String namespace_door_dormitorio = "P2"; 
String namespace_door_principal = "P3";
String namespace_luz_garage = "P4"; 
String namespace_luz_dormitorio = "L1";
String namespace_luz_sala = "L2";
String namespace_luz_cocina = "L3";
String namespace_movement = "M1";
String namespace_temperature = "T1_status";
String namespace_temperature_enable_auto = "T1_auto";
String namespace_temperature_limit = "T1_limit_temp";

Preferences preferences;

static void init_mqtt_client_id()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(
        mqtt_client_id,
        sizeof(mqtt_client_id),
        "esp32-%02X%02X%02X%02X%02X%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static bool i2c_write_packet(uint8_t addr, const I2CPacket &pkt)
{
    Serial.printf("[I2C_DBG] TX addr=0x%02X node_id=%d cmd=0x%02X data=0x%02X checksum=0x%02X\n",
                  addr, pkt.node_id, pkt.cmd, pkt.data, pkt.checksum);

    Wire.beginTransmission(addr);
    Wire.write(reinterpret_cast<const uint8_t *>(&pkt), PKT_SIZE);
    uint8_t result = Wire.endTransmission();

    Serial.printf("[I2C_DBG] endTransmission result=%d (0=OK, 1=too_long, 2=nack_addr, 3=nack_data, 4=error)\n", result);

    return result == 0;
}

static bool request_node_status(uint8_t addr, uint8_t node_id, I2CPacket &response)
{
    Serial.printf("[I2C_REQ] Solicitando estado a 0x%02X (node_id=%d)...\n", addr, node_id);

    I2CPacket request{node_id, CMD_STATUS, 0x00, 0x00};
    request.checksum = pkt_checksum(request);
    if (!i2c_write_packet(addr, request))
    {
        Serial.printf("[I2C_REQ] Fallo escribir a 0x%02X\n", addr);
        return false;
    }

    delay(2);
    unsigned long start = millis();

    // Intentar leer con timeout
    if (Wire.requestFrom(addr, static_cast<uint8_t>(PKT_SIZE)) != PKT_SIZE)
    {
        unsigned long elapsed = millis() - start;
        Serial.printf("[I2C_REQ] Timeout/NACK leyendo de 0x%02X (timeout=%lums)\n", addr, elapsed);
        return false;
    }

    response.node_id = Wire.read();
    response.cmd = Wire.read();
    response.data = Wire.read();
    response.checksum = Wire.read();

    unsigned long elapsed = millis() - start;
    Serial.printf("[I2C_REQ] Respuesta recibida de 0x%02X en %lums\n", addr, elapsed);

    return response.node_id == node_id && pkt_valid(response);
}

static void publish_json(const char *topic, const String payload, bool retained = true)
{
    mqtt->publish(topic, payload.c_str(), retained);
}

/**
 * Lee todos los paquetes de los devices que están en I2C
 */
static std::vector<I2CPacket> read_i2c_arduinos()
{
    std::vector<I2CPacket> read_packets;
    auto addresses = devices_controller->address_nodes();

    Serial.printf("[I2C_POLL] Leyendo %d devices...\n", addresses.size());

    for (I2CMetadata metadata : addresses)
    {
        I2CPacket response;
        bool result = request_node_status(metadata.address, metadata.node_id, response);
        if (result)
        {
            read_packets.push_back(response);
            Serial.printf("[I2C_POLL] ✓ 0x%02X respondió correctamente\n", metadata.address);
        }
        else
        {
            Serial.printf("[I2C_POLL] ✗ 0x%02X no respondió o checksum inválido\n", metadata.address);
        }
    }

    Serial.printf("[I2C_POLL] Lectura completa: %d/%d devices respondieron\n",
                  read_packets.size(), addresses.size());

    return read_packets;
}
static void publish_all_states()
{
    std::vector<I2CPacket> packets = read_i2c_arduinos();
    devices_controller->received_i2c(packets);
    std::vector<Publish> states = devices_controller->publish_action_mqtt();

    for (const Publish state : states)
    {
        Serial.print("Publicando \"");
        Serial.print(state.topic);
        Serial.print("\" con topic");
        Serial.println(state.payload);
        publish_json(state.topic.c_str(), state.payload);
    }
}

/**
 *Este es el callback que trabaja cuando recibimos mensaje en mqtt
 *Dependiendo del topic el programa configura el proyecto
 */
static void mqtt_message_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.printf("[MQTT_CB] Mensaje recibido en topic: %s\n", topic);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
    {
        Serial.printf("[MQTT] JSON invalido en %s: %s\n", topic, error.c_str());
        return;
    }

    devices_controller->subscriber_action_mqtt(String(topic), doc);
    std::vector<I2CBoxing> packet_updated = devices_controller->send_i2c();

    Serial.printf("[MQTT_CB] Enviando %d paquetes I2C\n", packet_updated.size());

    for (const I2CBoxing packet : packet_updated)
    {
        bool success = i2c_write_packet(packet.address, packet.pkt);
        Serial.printf("[MQTT_CB] Paquete a 0x%02X: %s\n", packet.address, success ? "OK" : "FAIL");
    }

    // Publicar confirmación de cambio de estado inmediatamente
    std::vector<Publish> states = devices_controller->publish_action_mqtt();
    Serial.printf("[MQTT_CB] Publicando %d estados\n", states.size());

    for (const Publish &state : states)
    {
        publish_json(state.topic.c_str(), state.payload);
        Serial.printf("[MQTT] Estado confirmado: %s = %s\n", state.topic.c_str(), state.payload.c_str());
    }
}

/**
 * Crea un conexión si el esp32 todavía no está conectado
 * con el broker
 * - Se subcribe a todos los topics
 * - Publica el estado actual de todos los topics
 * - Tiene un delay de 3 segundos
 */
static void mqtt_connect()
{
    if (mqtt->connected())
        return;

    static unsigned long last_attempt = 0;
    unsigned long now = millis();

    if (now - last_attempt < 500)
        return;
    last_attempt = now;

    if (mqtt->connect(mqtt_client_id, MQTT_USER, MQTT_PASSWORD))
    {
        // Suscribirse a todos los topics de los devices
        for (const String &topic : devices_controller->get_topics_devices())
        {
            mqtt->subscribe(topic.c_str());
            Serial.print("Suscribiendose a: ");
            Serial.println(topic);
            Serial.printf("[MQTT] Suscrito a: %s\n", topic.c_str());
        }

        // Publicar estado actual
        publish_all_states();
        return;
    }
    delay(500);
}

void save_status() {
    // Obtenemos el estado actual de todos los dispositivos
    bool state_door_principal = door_principal->get_state();
    bool state_door_garage = door_garage->get_state();
    bool state_door_dormitorio = door_dormitorio->get_state();
    bool state_luz_garage = luz_garage->get_state();
    bool state_luz_dormitorio = luz_dormitorio->get_state();
    bool state_luz_sala = luz_sala->get_state();
    bool state_luz_cocina = luz_cocina->get_state();
    bool state_movement = move_controller->get_state();
    bool state_temperature_state = temperature_controller->get_state();
    bool state_temperature_auto = temperature_controller->get_enable_auto();
    uint8_t state_temperature_limit = temperature_controller->get_temp_timit();

    // Guardamos el estado en la memoria persistente
    preferences.begin("my-app", false);
    preferences.putBool(namespace_door_principal.c_str(), state_door_principal);
    preferences.putBool(namespace_door_garage.c_str(), state_door_garage);
    preferences.putBool(namespace_door_dormitorio.c_str(), state_door_dormitorio);
    preferences.putBool(namespace_luz_garage.c_str(), state_luz_garage);
    preferences.putBool(namespace_luz_dormitorio.c_str(), state_luz_dormitorio);
    preferences.putBool(namespace_luz_sala.c_str(), state_luz_sala);
    preferences.putBool(namespace_luz_cocina.c_str(), state_luz_cocina);
    preferences.putBool(namespace_movement.c_str(), state_movement);
    preferences.putBool(namespace_temperature.c_str(), state_temperature_state);
    preferences.putBool(namespace_temperature_enable_auto.c_str(), state_temperature_auto);
    preferences.putUInt(namespace_temperature_limit.c_str(), state_temperature_limit);

    preferences.end();
}

void setup()
{
    Serial.begin(115200);

    preferences.begin("mi-app", false);
    preferences.end();
    /**
     * Agregamos los actuadores a actuatorscontroller
     */
    door_principal = new Actuator(UUID_PUERTA_PRINCIPAL, 0, TypeActuator::DOOR);
    door_garage = new Actuator(UUID_PUERTA_GARAGE, 1, TypeActuator::DOOR);
    door_dormitorio = new Actuator(UUID_PUERTA_DORMITORIO, 2, TypeActuator::DOOR);
    luz_garage = new Actuator(UUID_LUZ_GARAGE, 3, TypeActuator::LIGHT);
    luz_dormitorio = new Actuator(UUID_LUZ_DORMITORIO, 4, TypeActuator::LIGHT);
    luz_sala = new Actuator(UUID_LUZ_SALA, 5, TypeActuator::LIGHT);
    luz_cocina = new Actuator(UUID_LUZ_COCINA, 6, TypeActuator::LIGHT);

    preferences.begin(namespace_actuators.c_str(), true);
    bool state_door_principal = preferences.getBool(namespace_door_principal.c_str(), false);
    bool state_door_garage = preferences.getBool(namespace_door_garage.c_str(), false);
    bool state_door_dormitorio = preferences.getBool(namespace_door_dormitorio.c_str(), false);
    bool state_luz_garage = preferences.getBool(namespace_luz_garage.c_str(), false);
    bool state_luz_dormitorio = preferences.getBool(namespace_luz_dormitorio.c_str(), false);
    bool state_luz_sala = preferences.getBool(namespace_luz_sala.c_str(), false);
    bool state_luz_cocina = preferences.getBool(namespace_luz_cocina.c_str(), false);
    bool state_movement = preferences.getBool(namespace_movement.c_str(), false);
    bool state_temperature = preferences.getBool(namespace_temperature.c_str(), false);
    bool state_temperature_auto = preferences.getBool(namespace_temperature_enable_auto.c_str(), false);
    uint8_t state_temperature_limit = preferences.getUInt(namespace_temperature_limit.c_str(), 50);
    preferences.end();

    door_principal->change_state(state_door_principal);
    door_garage->change_state(state_door_garage);
    door_dormitorio->change_state(state_door_dormitorio);
    luz_garage->change_state(state_luz_garage);
    luz_dormitorio->change_state(state_luz_dormitorio);
    luz_sala->change_state(state_luz_sala);
    luz_cocina->change_state(state_luz_cocina);

    actuator_controller = new ActuatorsController(ADDR_ACTUATORS, NODE_ID_ACTUATORS);
    actuator_controller->add_actuators(door_principal);
    actuator_controller->add_actuators(door_garage);
    actuator_controller->add_actuators(door_dormitorio);
    actuator_controller->add_actuators(luz_garage);
    actuator_controller->add_actuators(luz_dormitorio);
    actuator_controller->add_actuators(luz_sala);
    actuator_controller->add_actuators(luz_cocina);

    temperature_controller = new TemperatureController(UUID_SENSOR_TEMPERATURE, ADDR_TEMPERATURE, NODE_ID_TEMPERATURE);
    temperature_controller->change_state(state_temperature_limit, state_temperature, state_temperature_auto);

    move_controller = new MoveController(UUID_SENSOR_MOVIMIENTO, ADDR_ALARM, NODE_ID_ALARM);
    move_controller->change_state(state_movement);
    /**
     * Agregamos los arduinos al controlador
     */
    Serial.print("Starting devices controlller...");
    devices_controller = new DevicesController();
    devices_controller->add_arduino(actuator_controller);
    devices_controller->add_arduino(temperature_controller);
    devices_controller->add_arduino(move_controller);

    Wire.begin();
    Serial.println("Wire ok");

    Serial.println("Connecting wifi...");
    Serial.println("Escaneando redes...");
    int n = WiFi.scanNetworks();

    for (int i = 0; i < n; i++)
    {
        Serial.print(WiFi.SSID(i));
        Serial.print(" | RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" | Canal: ");
        Serial.println(WiFi.channel(i));
    }

    Serial.print("Wifi SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("Password: ");
    Serial.println(WIFI_PASSWORD);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        Serial.print("Status: ");
        Serial.println(WiFi.status());
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(500);
    }

    Serial.println("\n Wifi conectado");
    mqtt = new PubSubClient(wifiClient);
    init_mqtt_client_id();
    mqtt->setServer(MQTT_HOST, MQTT_PORT);
    mqtt->setCallback(mqtt_message_callback);
    mqtt_connect();
}

static unsigned long last_mqtt_publish = 0;
static unsigned long last_i2c_sync = 0;
static unsigned long last_save_persistence = 0;

void loop()
{
    if (!mqtt->connected())
    {
        mqtt_connect();
    }
    mqtt->loop();

    unsigned long now = millis();

    // Leer estados desde Arduino cada 5 segundos (más tolerante con Arduino que no responden)
    if (now - last_status_poll > 5000)
    {
        Serial.println("[LOOP] Iniciando lectura I2C...");
        unsigned long read_start = millis();

        std::vector<I2CPacket> packets = read_i2c_arduinos();
        devices_controller->received_i2c(packets);

        unsigned long read_time = millis() - read_start;
        Serial.printf("[LOOP] Lectura I2C completada en %lums\n", read_time);

        last_status_poll = now;
    }

    // Publicar estados en MQTT cada 1 segundo (INDEPENDIENTE de lectura I2C)
    if (now - last_mqtt_publish > 1000)
    {
        Serial.println("[LOOP] Publicando estados MQTT...");

        std::vector<Publish> states = devices_controller->publish_action_mqtt();
        Serial.printf("[LOOP] Publicando %d estados\n", states.size());

        for (const Publish &state : states)
        {
            publish_json(state.topic.c_str(), state.payload);
        }
        last_mqtt_publish = now;
    }

    // Sincronizar estados con Arduino cada 5 segundos para garantizar consistencia
    if (now - last_i2c_sync > 5000)
    {
        Serial.println("[LOOP] Sincronizando estados con Arduino...");

        std::vector<I2CBoxing> packets = devices_controller->send_i2c();
        Serial.printf("[LOOP] Enviando %d paquetes de sincronización\n", packets.size());

        for (const I2CBoxing &packet : packets)
        {
            bool success = i2c_write_packet(packet.address, packet.pkt);
            if (!success)
            {
                Serial.printf("[LOOP] Fallo al sincronizar con 0x%02X\n", packet.address);
            }
        }
        last_i2c_sync = now;
    }

    if (now - last_save_persistence > 3000) // Guardar estado cada 10 segundos
    {
        Serial.println("[LOOP] Guardando estado en persistencia...");
        save_status();
        last_save_persistence = now;
    }
}
