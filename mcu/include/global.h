#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <task_handler.h>
// ===== STRUCT DATA =====
typedef struct {
    float temp;
    float humi;
    int tState;   // 0: normal, 1: warning, 2: critical
    int hState;
} SensorData_t;

// ===== QUEUE =====
extern QueueHandle_t xQueueLED;
extern QueueHandle_t xQueueNeo;
extern QueueHandle_t xQueueWeb;
extern QueueHandle_t xQueueTinyML;
extern QueueHandle_t xQueueCoreIot;
// ===== SEMAPHORE =====
extern SemaphoreHandle_t xSensorSem;
extern SensorData_t data;

extern bool isAPMode;
extern bool connecting;
extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern bool glob_is_anomaly;
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
#endif