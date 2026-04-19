#include "camera.h"
#include "Arduino.h"

// Pinout AI Thinker ESP32-CAM
#define PWDN_GPIO  32
#define RESET_GPIO -1
#define XCLK_GPIO  0
#define SIOD_GPIO  26
#define SIOC_GPIO  27
#define Y9_GPIO    35
#define Y8_GPIO    34
#define Y7_GPIO    39
#define Y6_GPIO    38
#define Y5_GPIO    37
#define Y4_GPIO    36
#define Y3_GPIO    21
#define Y2_GPIO    19
#define VSYNC_GPIO 25
#define HREF_GPIO  23
#define PCLK_GPIO  22

static bool _active = false;

bool camera_init() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO;
    config.pin_d1       = Y3_GPIO;
    config.pin_d2       = Y4_GPIO;
    config.pin_d3       = Y5_GPIO;
    config.pin_d4       = Y6_GPIO;
    config.pin_d5       = Y7_GPIO;
    config.pin_d6       = Y8_GPIO;
    config.pin_d7       = Y9_GPIO;
    config.pin_xclk     = XCLK_GPIO;
    config.pin_pclk     = PCLK_GPIO;
    config.pin_vsync    = VSYNC_GPIO;
    config.pin_href     = HREF_GPIO;
    config.pin_sccb_sda = SIOD_GPIO;
    config.pin_sccb_scl = SIOC_GPIO;
    config.pin_pwdn     = PWDN_GPIO;
    config.pin_reset    = RESET_GPIO;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        config.frame_size   = FRAMESIZE_VGA;   // 640x480
        config.jpeg_quality = 12;
        config.fb_count     = 2;
    } else {
        config.frame_size   = FRAMESIZE_QVGA;  // 320x240
        config.jpeg_quality = 20;
        config.fb_count     = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CAM] Error init: 0x%x\n", err);
        return false;
    }

    _active = true;
    Serial.println("[CAM] Inicializada");
    return true;
}

void camera_deinit() {
    esp_camera_deinit();
    _active = false;
    Serial.println("[CAM] Apagada");
}

bool camera_is_active() {
    return _active;
}

void camera_set_active(bool active) {
    if (active && !_active) camera_init();
    if (!active && _active) camera_deinit();
}

camera_fb_t* camera_capture() {
    if (!_active) return nullptr;
    return esp_camera_fb_get();
}