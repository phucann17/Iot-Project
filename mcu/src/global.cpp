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

String WIFI_SSID = "iPhone";
String WIFI_PASS = "11111111";
String CORE_IOT_SERVER = "app.coreiot.io";
String CORE_IOT_PORT   = "1883";
String CORE_IOT_TOKEN  = "vQX7xBRJgEfRNJntXIPc";

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "abcde";
String wifi_password = "123456789";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();