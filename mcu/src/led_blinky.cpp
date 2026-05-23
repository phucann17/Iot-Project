#include "led_blinky.h"
#include "global.h"
void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  SensorData_t data;
  int tempMode = 0;
  
  while(1) {      
    // đọc trạng thái nhiệt độ 
    if(xSensorSem != NULL) {
        if (xSemaphoreTake(xSensorSem, 100) == pdTRUE) {
            if (xQueueReceive(xQueueLED, &data, 0) == pdTRUE) {
              tempMode = data.tState;
              xSemaphoreGive(xSensorSem);
            }
    }
    switch(tempMode){
      case 0: // mát: nháy chậm (2s)
        Serial.println("[LED] SLOW 2s\n");
        digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
        vTaskDelay(100);
        digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
        vTaskDelay(1900);
        break;
      case 1: // ấm nháy chậm (1s)
        Serial.println("[LED] SLOW 1s\n");
        digitalWrite(LED_GPIO, HIGH);
        vTaskDelay(500);
        digitalWrite(LED_GPIO, LOW);
        vTaskDelay(500);
        break;
      case 2: // nóng: nháy nhanh (0.5s)
        Serial.println("[LED] FAST 0.5s\n");
        digitalWrite(LED_GPIO, HIGH);
        vTaskDelay(250);
        digitalWrite(LED_GPIO, LOW);
        vTaskDelay(250);
        break;
    }
    
  }
}
}