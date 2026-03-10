#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// ─────────────────────────────────────────
//  CONFIGURA TU RED AQUÍ
// ─────────────────────────────────────────
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// ─────────────────────────────────────────
//  PINES — AI Thinker ESP32-CAM
// ─────────────────────────────────────────
#define LED_FLASH   4   // Flash LED blanco (built-in)
#define LED_ROJO    33  // LED rojo (built-in, activo en LOW)

// ─────────────────────────────────────────
//  CONFIGURACIÓN CÁMARA — AI Thinker
// ─────────────────────────────────────────
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

bool flashOn  = false;
bool ledRojoOn = false;

// ─────────────────────────────────────────
//  PÁGINA WEB EMBEBIDA
// ─────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32-CAM Control</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Exo+2:wght@300;600;800&display=swap');

  :root {
    --green: #00ff88;
    --red: #ff3355;
    --amber: #ffaa00;
    --bg: #080c10;
    --panel: #0d1420;
    --border: rgba(0,255,136,0.15);
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }

  body {
    background: var(--bg);
    color: var(--green);
    font-family: 'Share Tech Mono', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 24px 16px;
    background-image:
      radial-gradient(ellipse at 20% 0%, rgba(0,255,136,0.06) 0%, transparent 60%),
      radial-gradient(ellipse at 80% 100%, rgba(0,100,255,0.05) 0%, transparent 60%);
  }

  header {
    width: 100%;
    max-width: 720px;
    display: flex;
    align-items: baseline;
    gap: 12px;
    margin-bottom: 28px;
    border-bottom: 1px solid var(--border);
    padding-bottom: 14px;
  }

  header h1 {
    font-family: 'Exo 2', sans-serif;
    font-weight: 800;
    font-size: 1.6rem;
    letter-spacing: 0.08em;
    color: #fff;
  }

  header span {
    font-size: 0.75rem;
    color: var(--amber);
    letter-spacing: 0.2em;
  }

  #status-bar {
    width: 100%;
    max-width: 720px;
    display: flex;
    gap: 20px;
    margin-bottom: 20px;
    font-size: 0.72rem;
    color: rgba(0,255,136,0.5);
    letter-spacing: 0.12em;
  }

  .dot {
    display: inline-block;
    width: 7px; height: 7px;
    border-radius: 50%;
    background: var(--green);
    margin-right: 6px;
    box-shadow: 0 0 6px var(--green);
    animation: pulse 2s infinite;
  }

  @keyframes pulse {
    0%,100% { opacity: 1; }
    50% { opacity: 0.3; }
  }

  /* ── Stream ── */
  .stream-wrap {
    width: 100%;
    max-width: 720px;
    aspect-ratio: 4/3;
    background: #000;
    border: 1px solid var(--border);
    position: relative;
    overflow: hidden;
    margin-bottom: 20px;
  }

  .stream-wrap::before {
    content: '';
    position: absolute; inset: 0;
    background: repeating-linear-gradient(
      0deg, transparent, transparent 2px,
      rgba(0,255,136,0.015) 2px, rgba(0,255,136,0.015) 4px
    );
    pointer-events: none;
    z-index: 2;
  }

  #stream {
    width: 100%; height: 100%;
    object-fit: cover;
    display: block;
  }

  .stream-label {
    position: absolute;
    top: 10px; left: 12px;
    font-size: 0.65rem;
    color: var(--green);
    letter-spacing: 0.18em;
    z-index: 3;
    opacity: 0.7;
  }

  /* ── Controls ── */
  .controls {
    width: 100%;
    max-width: 720px;
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 14px;
  }

  .ctrl-card {
    background: var(--panel);
    border: 1px solid var(--border);
    padding: 18px 20px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }

  .ctrl-info {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .ctrl-name {
    font-family: 'Exo 2', sans-serif;
    font-weight: 600;
    font-size: 0.85rem;
    color: #fff;
    letter-spacing: 0.05em;
  }

  .ctrl-state {
    font-size: 0.7rem;
    letter-spacing: 0.15em;
  }

  .state-on  { color: var(--green); }
  .state-off { color: rgba(255,255,255,0.25); }

  /* Toggle switch */
  .toggle {
    position: relative;
    width: 52px; height: 28px;
    flex-shrink: 0;
  }

  .toggle input { opacity: 0; width: 0; height: 0; }

  .slider {
    position: absolute; inset: 0;
    background: rgba(255,255,255,0.08);
    border: 1px solid rgba(255,255,255,0.12);
    cursor: pointer;
    transition: 0.25s;
  }

  .slider::before {
    content: '';
    position: absolute;
    height: 18px; width: 18px;
    left: 4px; bottom: 4px;
    background: rgba(255,255,255,0.3);
    transition: 0.25s;
  }

  input:checked + .slider {
    background: rgba(0,255,136,0.15);
    border-color: var(--green);
    box-shadow: 0 0 12px rgba(0,255,136,0.2);
  }

  input:checked + .slider::before {
    transform: translateX(24px);
    background: var(--green);
    box-shadow: 0 0 8px var(--green);
  }

  /* snapshot button */
  .snap-card {
    background: var(--panel);
    border: 1px solid var(--border);
    padding: 18px 20px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }

  #snap-btn {
    background: transparent;
    border: 1px solid var(--amber);
    color: var(--amber);
    font-family: 'Share Tech Mono', monospace;
    font-size: 0.78rem;
    letter-spacing: 0.15em;
    padding: 8px 18px;
    cursor: pointer;
    transition: 0.2s;
  }

  #snap-btn:hover {
    background: rgba(255,170,0,0.12);
    box-shadow: 0 0 12px rgba(255,170,0,0.2);
  }

  #snap-btn:active { transform: scale(0.97); }

  footer {
    margin-top: 28px;
    font-size: 0.65rem;
    color: rgba(0,255,136,0.25);
    letter-spacing: 0.2em;
  }
</style>
</head>
<body>

<header>
  <h1>ESP32-CAM</h1>
  <span>CONTROL PANEL v1.0</span>
</header>

<div id="status-bar">
  <span><span class="dot"></span>STREAM LIVE</span>
  <span id="ip-label">IP: —</span>
</div>

<div class="stream-wrap">
  <span class="stream-label">CAM_0 // MJPEG</span>
  <img id="stream" src="/stream" alt="stream">
</div>

<div class="controls">

  <div class="ctrl-card">
    <div class="ctrl-info">
      <span class="ctrl-name">⚡ FLASH LED</span>
      <span class="ctrl-state state-off" id="flash-state">OFF</span>
    </div>
    <label class="toggle">
      <input type="checkbox" id="flash-toggle" onchange="toggle('flash', this.checked)">
      <span class="slider"></span>
    </label>
  </div>

  <div class="ctrl-card">
    <div class="ctrl-info">
      <span class="ctrl-name">🔴 LED ROJO</span>
      <span class="ctrl-state state-off" id="led-state">OFF</span>
    </div>
    <label class="toggle">
      <input type="checkbox" id="led-toggle" onchange="toggle('led', this.checked)">
      <span class="slider"></span>
    </label>
  </div>

  <div class="snap-card">
    <div class="ctrl-info">
      <span class="ctrl-name">📷 CAPTURA</span>
      <span class="ctrl-state state-off">JPEG SNAPSHOT</span>
    </div>
    <button id="snap-btn" onclick="snapshot()">CAPTURAR</button>
  </div>

</div>

<footer>ESP32-CAM // AI THINKER // 2.4GHz</footer>

<script>
  // Mostrar IP actual
  document.getElementById('ip-label').textContent = 'IP: ' + location.hostname;

  async function toggle(device, on) {
    const url = `/${device}/${on ? 'on' : 'off'}`;
    await fetch(url);
    const stateEl = document.getElementById(device === 'flash' ? 'flash-state' : 'led-state');
    stateEl.textContent = on ? 'ON' : 'OFF';
    stateEl.className   = 'ctrl-state ' + (on ? 'state-on' : 'state-off');
  }

  async function snapshot() {
    const btn = document.getElementById('snap-btn');
    btn.textContent = 'CAPTURANDO...';
    try {
      const resp = await fetch('/capture');
      const blob = await resp.blob();
      const url  = URL.createObjectURL(blob);
      const a    = document.createElement('a');
      a.href     = url;
      a.download = `esp32cam_${Date.now()}.jpg`;
      a.click();
    } catch(e) { alert('Error al capturar'); }
    btn.textContent = 'CAPTURAR';
  }
</script>
</body>
</html>
)rawliteral";

// ─────────────────────────────────────────
//  INIT CÁMARA
// ─────────────────────────────────────────
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
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

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
  WiFi.begin(WIFI_SSID, WIFI_PASS);
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
  server.on("/",           HTTP_GET, handleRoot);
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