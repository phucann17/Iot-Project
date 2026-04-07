#include <Arduino.h>

void TaskLEDControl(void *pvParameters) {
  pinMode(GPIO_NUM_33, OUTPUT);
  int ledState = 0;

  while(1) {
    if (ledState == 0) {
      digitalWrite(GPIO_NUM_33, HIGH);
    } else {
      digitalWrite(GPIO_NUM_33, LOW);
    }

    ledState = 1 - ledState;

    vTaskDelay(pdMS_TO_TICKS(2000)); // FIX
  }
}

void setup() {
  Serial.begin(115200);
  xTaskCreate(TaskLEDControl, "LED Control", 2048, NULL, 2, NULL);
}

void loop() {}