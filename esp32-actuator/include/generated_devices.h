#pragma once
#include <Arduino.h>

struct DeviceMetadata {
    const char* name;
    const char* uuid;
    const char* type;
};

#define UUID_CAMERA "d2831400-bf96-4bb8-9175-fbe3a5ac47b5"
#define UUID_PUERTA_PRINCIPAL "2b55ccca-ec7e-41de-ab8c-eaa2a0f7de91"
#define UUID_PUERTA_GARAGE "a6ffdd49-ab85-4393-8f21-0f74ff5cd5ad"
#define UUID_PUERTA_DORMITORIO "d9cf9fea-f6f4-45d0-94a2-6b425cddde3a"
#define UUID_LUZ_GARAGE "8bc250eb-393f-4a25-82db-793c3c65d89f"
#define UUID_LUZ_DORMITORIO "750b4b38-55bc-468a-9b08-f0ec8bacc44e"
#define UUID_LUZ_SALA "ed91389c-cee0-4b9b-ad74-dcae0b5f25ab"
#define UUID_LUZ_COCINA "fd5e8be1-bff7-4067-972b-7357b2ed2c03"
#define UUID_SENSOR_TEMPERATURE "73724302-3a02-407f-be98-2c1ddd3d4931"
#define UUID_SENSOR_MOVIMIENTO "454fd10f-65e2-4585-9afb-92ba7816e3eb"

static constexpr DeviceMetadata DEVICE_METADATA[] = {
    { "CAMERA", UUID_CAMERA, "camera" },
    { "PUERTA_PRINCIPAL", UUID_PUERTA_PRINCIPAL, "door" },
    { "PUERTA_GARAGE", UUID_PUERTA_GARAGE, "door" },
    { "PUERTA_DORMITORIO", UUID_PUERTA_DORMITORIO, "door" },
    { "LUZ_GARAGE", UUID_LUZ_GARAGE, "light" },
    { "LUZ_DORMITORIO", UUID_LUZ_DORMITORIO, "light" },
    { "LUZ_SALA", UUID_LUZ_SALA, "light" },
    { "LUZ_COCINA", UUID_LUZ_COCINA, "light" },
    { "SENSOR_TEMPERATURE", UUID_SENSOR_TEMPERATURE, "thermostat" },
    { "SENSOR_MOVIMIENTO", UUID_SENSOR_MOVIMIENTO, "movement" },
};

static constexpr size_t DEVICE_METADATA_COUNT = sizeof(DEVICE_METADATA) / sizeof(DeviceMetadata);
