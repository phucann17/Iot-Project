#include <Arduino.h>
#include "global.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
// #include "mainserver.h"
// #include "tinyml.h"
// #include "coreiot.h"

// include task
// #include "task_check_info.h"
// #include "task_toogle_boot.h"
// #include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);

  // check_info_File(0);
  // // --- 1. KHỞI TẠO SEMAPHORE ---
  // if (xBinarySemaphoreInternet == NULL) {
  //     xBinarySemaphoreInternet = xSemaphoreCreateBinary();
  //     xSemaphoreGive(xBinarySemaphoreInternet);
  // }
  
  // xCloudStateSemaphore = xSemaphoreCreateBinary();
  // if (xCloudStateSemaphore != NULL) {
  //     xSemaphoreGive(xCloudStateSemaphore);
  // }
  // // ---------------------------------------------
  // // --- KHỞI TẠO SEMAPHORE CHO CẢM BIẾN (THÊM MỚI) ---
  // xSensorStateSemaphore = xSemaphoreCreateBinary();
  // if (xSensorStateSemaphore != NULL) {
  //     xSemaphoreGive(xSensorStateSemaphore); // Mở khóa ngay
  // }
  // Create queue
  xQueueLED = xQueueCreate(5, sizeof(SensorData_t));
  xQueueNeo = xQueueCreate(5, sizeof(SensorData_t));
  xQueueWeb = xQueueCreate(5, sizeof(SensorData_t));
  xQueueTinyML = xQueueCreate(5, sizeof(SensorData_t));
  xQueueCoreIot = xQueueCreate(5, sizeof(SensorData_t));
  if (xQueueLED == NULL || xQueueNeo == NULL || xQueueWeb == NULL || xQueueTinyML == NULL || xQueueCoreIot == NULL) {
      Serial.println("Queue create FAILED!");
  } else {
      Serial.println("Queue create OK");
  }

  xSensorSem = xSemaphoreCreateBinary();

  if (xSensorSem == NULL) {
      Serial.println("Semaphore create FAILED");
  }
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096,NULL, 2, NULL);
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  // //xTaskCreate(main_server_task, "Task Main Server" ,8192  ,NULL  ,2 , NULL);
  xTaskCreate(task_webserver, "Task Web Server", 8192, NULL, 2, NULL);
  // xTaskCreate( tiny_ml_task, "Tiny ML Task" ,8192  ,NULL  ,2 , NULL);
  xTaskCreate(task_core_iot, "CoreIOT Task" ,4096  ,NULL  ,2 , NULL);
  // //xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop()
{
  // if (check_info_File(1))
  // {
  //   if (!Wifi_reconnect())
  //   {
  //     Webserver_stop();
  //   }
  //   else
  //   {
  //     //CORE_IOT_reconnect();
  //   }
  // }
  // Webserver_reconnect();
  // vTaskDelay(10); // Thêm delay nhẹ để tránh watchdog nếu loop chạy quá nhanh
}