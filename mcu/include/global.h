#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "global.h"
// ================== STRUCT TRUYỀN DATA ==================
typedef struct {
    float temp;
    float humi;
    int tState;   // 0: normal, 1: warning, 2: critical
    int hState;
} SensorData_t;

// ================== QUEUE ==================
extern QueueHandle_t xSensorQueue;

// ================== SEMAPHORE ==================
extern SemaphoreHandle_t semLCD;
extern SemaphoreHandle_t semLED;
extern SemaphoreHandle_t semNeo;

// ================== WIFI / INTERNET ==================
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// ================== CLOUD ==================
extern SemaphoreHandle_t xCloudStateSemaphore;

#endif