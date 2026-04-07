#include "led_blinky.h"

void led_blinky(void *pvParameters){
    int tempMode = 0;
    SensorData_t data;

    Serial.printf("[LED TASK] Started");

    while(1) {

        // Đợi sensor báo có data
        if (xSemaphoreTake(semLED, portMAX_DELAY) == pdTRUE) {

            // Lấy data từ queue
            if (xQueueReceive(xSensorQueue, &data, 0) == pdTRUE) {
                tempMode = data.tState;

                // DEBUG CHÍNH
                Serial.printf("[LED] Temp=%.1f | Mode=%d\n", data.temp, tempMode);
            }

            // Giả lập blink bằng log
            switch(tempMode){
                case 0:
                    Serial.println("[LED] BLINK SLOW (2s)");
                    break;

                case 1:
                    Serial.println("[LED] BLINK MEDIUM (1s)");
                    break;

                case 2:
                    Serial.println("[LED] BLINK FAST (0.5s)");
                    break;
            }
        }
    }
}