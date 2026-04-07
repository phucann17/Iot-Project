#include "temp_humi_monitor.h"
#include <Wire.h>
#include <stdlib.h>
#include <time.h>

#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12

void temp_humi_monitor(void *pvParameters) {

    srand((unsigned int)time(NULL));

    SensorData_t data;

    Serial.println("[Sensor] Init Done.");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1) {

        // --- RANDOM DATA ---
        data.temp = 20.0 + ((float)rand() / RAND_MAX) * (40.0 - 20.0);
        data.humi = 40.0 + ((float)rand() / RAND_MAX) * (90.0 - 40.0);

        // --- PHÂN LOẠI ---
        data.tState = 0;
        if (data.temp >= 30 && data.temp < 35) data.tState = 1;
        else if (data.temp >= 35) data.tState = 2;

        data.hState = 0;
        if (data.humi >= 60 && data.humi < 80) data.hState = 1;
        else if (data.humi >= 80) data.hState = 2;

        Serial.printf("Temp: %.1f | Humi: %.1f | tState=%d | hState=%d\n",
                      data.temp, data.humi, data.tState, data.hState);

        // --- GỬI DATA ---
        if (xSensorQueue != NULL) {
            xQueueOverwrite(xSensorQueue, &data);
        }

        // --- BÁO TỪNG TASK ---
        if (semLCD != NULL) xSemaphoreGive(semLCD);
        if (semLED != NULL) xSemaphoreGive(semLED);
        if (semNeo != NULL) xSemaphoreGive(semNeo);

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}