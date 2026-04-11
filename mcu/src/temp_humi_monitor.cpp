#include "temp_humi_monitor.h"
#include "DHT20.h"
#include "global.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12
DHT20 dht20;
LiquidCrystal_I2C lcd(33, 16, 2); 

void temp_humi_monitor(void *pvParameters) {
    // --- 1. KHỞI TẠO LCD TRƯỚC (QUAN TRỌNG) ---
    // Để LCD khởi tạo Wire mặc định trước, tránh reset cấu hình của mình
    // Wire.begin(SDA_PIN, SCL_PIN); 
    // lcd.begin(); 
    // lcd.backlight();
    // lcd.print("Init Sensor...");

    // --- 2. CẤU HÌNH LẠI I2C CHO ĐÚNG CHÂN ---
    // Ghi đè cấu hình chân 11, 12 sau khi LCD đã init xong
    
    
    // --- 3. KHỞI TẠO DHT20 ---
    // Lúc này DHT20 sẽ dùng đúng chân 11, 12 mà ta vừa set
    // dht20.begin();
    SensorData_t data;
    Serial.println("[Sensor] Init Done.");
    // vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1) {
        // --- ĐỌC CẢM BIẾN ---
        // int status = dht20.read();
        
        // if (status != DHT20_OK) {
        //     Serial.println("DHT20 Error! Resetting I2C...");
        //     // Nếu lỗi, thử reset lại I2C
        //     Wire.begin(SDA_PIN, SCL_PIN);
        //     dht20.begin();
        //     vTaskDelay(2000 / portTICK_PERIOD_MS);
        //     continue;
        // }

        // float temp = dht20.getTemperature();
        // float humi = dht20.getHumidity();
        data.temp = 20.0 + (rand() % 200) / 10.0;
        data.humi = 40.0 + (rand() % 600) / 10.0;



        // --- LOGIC PHÂN LOẠI ---
        if (data.temp < 30) data.tState = 0;
        else if (data.temp >= 30 && data.temp < 35) data.tState = 1;
        else data.tState = 2;

        if (data.humi < 60) data.hState = 0;
        else if (data.humi >= 60 && data.humi < 80) data.hState = 1;
        else data.hState = 2;      
        Serial.printf("Temp: %.1f C (%d) | Humi: %.1f %% (%d)\n",
              data.temp, data.tState,
              data.humi, data.hState);
        // Gửi Semaphore (Đã sửa tên biến currentHumiState)
        
        // ===== GỬI QUEUE =====
        xQueueSend(xQueueLED, &data, 0);
        xQueueSend(xQueueNeo, &data, 0);
        xQueueSend(xQueueWeb, &data, 0);
        xQueueSend(xQueueCoreIot, &data, 0);
        // Serial.printf("LED Queue: %p\n", xQueueLED);
        // Serial.printf("Neo Queue: %p\n", xQueueNeo);
        // Serial.printf("LCD Queue: %p\n", xQueueLCD);
        // ===== TRIGGER TASK =====
        xSemaphoreGive(xSensorSem);

        // // --- HIỂN THỊ LCD ---
        // lcd.setCursor(0, 0);
        // lcd.print("T:"); lcd.print(temp, 1); lcd.print(" H:"); lcd.print(humi, 0); lcd.print("%");
        
        // lcd.setCursor(0, 1);
        // if (glob_is_anomaly) {
        //     lcd.print("AI: ANOMALY!    ");
        // } else if (tState == 2) {
        //     lcd.print("WARN: HIGH TEMP!");
        // } else {
        //     lcd.print("System Normal   ");
        // }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}