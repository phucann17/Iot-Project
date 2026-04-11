#include "global.h"

// ===== QUEUE =====
QueueHandle_t xQueueLED = NULL;
QueueHandle_t xQueueNeo = NULL;
QueueHandle_t xQueueWeb = NULL;
QueueHandle_t xQueueCoreIot = NULL;
// ===== SEMAPHORE =====
SemaphoreHandle_t xSensorSem = NULL;

bool isAPMode = false;
bool connecting = true;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String WIFI_SSID = "NHU";
String WIFI_PASS = "23042007";
String CORE_IOT_SERVER = "app.coreiot.io";
String CORE_IOT_PORT   = "1883";
String CORE_IOT_TOKEN  = "vQX7xBRJgEfRNJntXIPc";

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "abcde";
String wifi_password = "123456789";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();