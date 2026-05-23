#include "global.h"

// ===== QUEUE =====
QueueHandle_t xQueueLED = NULL;
QueueHandle_t xQueueNeo = NULL;
QueueHandle_t xQueueWeb = NULL;
QueueHandle_t xQueueCoreIot = NULL;
QueueHandle_t xQueueTinyML = NULL;
// ===== SEMAPHORE =====
SemaphoreHandle_t xSensorSem = NULL;

bool isAPMode = false;
bool connecting = true;
bool glob_is_anomaly = false;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String WIFI_SSID = "YOUR_WIFI_NETWORK";           // Replace with your WiFi SSID
String WIFI_PASS = "YOUR_WIFI_PASSWORD";          // Replace with your WiFi password
String CORE_IOT_SERVER = "your-mqtt-broker.com";  // Replace with your MQTT broker
String CORE_IOT_PORT   = "1883";
String CORE_IOT_TOKEN  = "YOUR_DEVICE_TOKEN";     // Replace with your device token

String ssid = "ESP32-Config";                     // AP Mode SSID
String password = "ESP32Config@2024";             // AP Mode Password (change for security)
String wifi_ssid = "YOUR_WIFI_NETWORK";           // Backup WiFi SSID
String wifi_password = "YOUR_WIFI_PASSWORD";      // Backup WiFi password
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();