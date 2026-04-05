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

static Actuator door_principal(UUID_PUERTA_PRINCIPAL, 0, TypeActuator::DOOR);
static Actuator door_garage(UUID_PUERTA_PRINCIPAL, 1, TypeActuator::DOOR);
static Actuator door_dormitorio(UUID_PUERTA_PRINCIPAL, 2, TypeActuator::DOOR);
static Actuator luz_garage(UUID_PUERTA_PRINCIPAL, 3, TypeActuator::DOOR);
static Actuator luz_dormitorio(UUID_PUERTA_PRINCIPAL, 4, TypeActuator::DOOR);
static Actuator luz_sala(UUID_PUERTA_PRINCIPAL, 5, TypeActuator::DOOR);
static Actuator luz_cocina(UUID_PUERTA_PRINCIPAL, 6, TypeActuator::DOOR);

constexpr uint8_t ADDR_ALARM = 0x08;
constexpr uint8_t ADDR_ACTUATORS = 0x09;
constexpr uint8_t ADDR_TEMPERATURE = 0x0A;

constexpr uint8_t NODE_ID_ALARM = 1;
constexpr uint8_t NODE_ID_ACTUATORS = 2;
constexpr uint8_t NODE_ID_TEMPERATURE = 3;
static ActuatorsController actuator_controller(ADDR_ACTUATORS, NODE_ID_ACTUATORS);
static TemperatureController temperature_controller(UUID_SENSOR_TEMPERATURE, ADDR_TEMPERATURE, NODE_ID_TEMPERATURE);
static MoveController move_controller(UUID_SENSOR_MOVIMIENTO, ADDR_ALARM, NODE_ID_ALARM);
static DevicesController devices_controller;


static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static char mqtt_client_id[32] = {0};
static unsigned long last_status_poll = 0;

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
    Wire.beginTransmission(addr);
    Wire.write(reinterpret_cast<const uint8_t *>(&pkt), PKT_SIZE);
    return Wire.endTransmission() == 0;
}

static bool i2c_read_packet(uint8_t addr, I2CPacket &pkt)
{
    if (Wire.requestFrom(addr, static_cast<uint8_t>(PKT_SIZE)) != PKT_SIZE)
    {
        return false;
    }
    pkt.node_id = Wire.read();
    pkt.cmd = Wire.read();
    pkt.data = Wire.read();
    pkt.checksum = Wire.read();
    return pkt_valid(pkt);
}

static bool request_node_status(uint8_t addr, uint8_t node_id, I2CPacket &response)
{

    I2CPacket request{node_id, CMD_STATUS, 0x00, 0x00};
    request.checksum = pkt_checksum(request);
    if (!i2c_write_packet(addr, request))
    {
        return false;
    }
    delay(10);
    if (!i2c_read_packet(addr, response))
    {
        return false;
    }
    return response.node_id == node_id;
}

static void publish_json(const char *topic, const String payload, bool retained = true)
{
    mqtt.publish(topic, payload.c_str(), retained);
}

/**
 * Lee todos los paquetes de los devices que están en I2C
 */
static std::vector<I2CPacket> read_i2c_arduinos() {
    std::vector<I2CPacket> read_packets;
    for(I2CMetadata metadata: devices_controller.address_nodes()) {
        I2CPacket response;
        bool result = request_node_status(metadata.address, metadata.node_id, response);
        if(result) {
            read_packets.push_back(response);
        }
    }
    return read_packets;
}
static void publish_all_states()
{
    std::vector<I2CPacket> packets = read_i2c_arduinos();
    devices_controller.received_i2c(packets);
    std::vector<Publish> states =  devices_controller.publish_action_mqtt();

    for(const Publish state: states) {
        publish_json(state.topic.c_str(), state.payload);
    }
}

/**
 *Este es el callback que trabaja cuando recibimos mensaje en mqtt
 *Dependiendo del topic el programa configura el proyecto
 */
static void mqtt_message_callback(char *topic, byte *payload, unsigned int length)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
    {
        Serial.printf("[MQTT] JSON invalido en %s: %s\n", topic, error.c_str());
        return;
    }

    devices_controller.subscriber_action_mqtt(String(topic), doc);
    std::vector<I2CBoxing> packet_updated = devices_controller.send_i2c();

    for(const I2CBoxing packet: packet_updated) {
        i2c_write_packet(packet.address, packet.pkt);
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
    while (!mqtt.connected())
    {
        if (mqtt.connect(mqtt_client_id, MQTT_USER, MQTT_PASSWORD))
        {
            // Suscribirse a todos los topics de los devices
            std::vector<String> topics = devices_controller.get_topics_devices();
            for (const String &topic : topics) {
                mqtt.subscribe(topic.c_str());
                Serial.printf("[MQTT] Suscrito a: %s\n", topic.c_str());
            }
            
            // Publicar estado actual
            publish_all_states();
            return;
        }
        delay(3000);
    }
}

void setup()
{
    /**
     * Agregamos los actuadores a actuatorscontroller
     */
    actuator_controller.add_actuators(&door_principal);
    actuator_controller.add_actuators(&door_garage);
    actuator_controller.add_actuators(&door_dormitorio);
    actuator_controller.add_actuators(&luz_garage);
    actuator_controller.add_actuators(&luz_dormitorio);
    actuator_controller.add_actuators(&luz_sala);
    actuator_controller.add_actuators(&luz_cocina);

    /**
     * Agregamos los arduinos al controlador
     */
    devices_controller.add_arduino(actuator_controller);
    devices_controller.add_arduino(temperature_controller);
    devices_controller.add_arduino(move_controller);

    Serial.begin(115200);
    Wire.begin();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    init_mqtt_client_id();
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqtt_message_callback);
    mqtt_connect();
}

void loop()
{
    if (!mqtt.connected())
    {
        mqtt_connect();
    }
    mqtt.loop();
    if (millis() - last_status_poll > 30000)
    {
        publish_all_states();
        last_status_poll = millis();
    }
}
