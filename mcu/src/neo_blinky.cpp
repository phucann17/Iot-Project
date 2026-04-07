#include "neo_blinky.h" // Chứa NEO_PIN (45) và LED_COUNT (1) của thầy
#include "global.h"     // Chứa biến độ ẩm và Semaphore
#include <Adafruit_NeoPixel.h>

void neo_blinky(void *pvParameters) {
    // 1. KHỞI TẠO (Giữ nguyên code thầy)
    // Sử dụng đúng NEO_PIN và LED_COUNT đã define trong neo_blinky.h
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    
    strip.begin();
    strip.setBrightness(50); // Thêm dòng này để đèn đỡ chói mắt
    strip.clear();
    strip.show();

    int humiMode = 0; // Biến lưu trạng thái độ ẩm (0: Khô, 1: Ổn, 2: Ẩm)
    SensorData_t data;
    while(1) {
        // 2. LOGIC ĐỒ ÁN (Thay thế đoạn nháy đỏ đơn giản của thầy)
        
        // BƯỚC A: Đọc trạng thái độ ẩm an toàn bằng Semaphore
        if (xSensorStateSemaphore != NULL) {
            if (xSemaphoreTake(xSensorStateSemaphore, 100) == pdTRUE) {
                humiMode = currentHumiState; // Lấy trạng thái từ Task Sensor gửi sang
                xSemaphoreGive(xSensorStateSemaphore);
            }
        }

        if (xSemaphoreTake(semNeo, 0) == pdTRUE) {   // không block
            if (xQueueReceive(xSensorQueue, &data, 0) == pdTRUE) {
                humiMode = data.hState;
                // DEBUG
                Serial.printf("[NEO] Humi=%.1f | Mode=%d\n",
                              data.humi, humiMode);
            }
        }
        // BƯỚC B: Hiển thị màu theo độ ẩm (Yêu cầu Task 2)
        // Bạn có thể chỉnh màu tùy thích theo ý thầy
        switch (humiMode) {
            case 0: // KHÔ (<60%) -> Màu ĐỎ (Cảnh báo)
                strip.setPixelColor(0, strip.Color(255, 0, 0)); 
                Serial.println("[NEO] RED (DRY)");
                break;
                
            case 1: // ỔN ĐỊNH (60-80%) -> Màu XANH LÁ (An toàn)
                strip.setPixelColor(0, strip.Color(0, 255, 0)); 
                Serial.println("[NEO] GREEN (NORMAL)");
                break;
                
            case 2: // ẨM ƯỚT (>80%) -> Màu XANH DƯƠNG (Nước)
                strip.setPixelColor(0, strip.Color(0, 0, 255)); 
                Serial.println("[NEO] BLUE (HUMID)");
                break;
                
            default:
                strip.clear(); // Tắt nếu lỗi
                Serial.println("[NEO] OFF");
                break;
        }

        strip.show(); // Cập nhật đèn
        
        vTaskDelay(500 / portTICK_PERIOD_MS); // Chờ 0.5s rồi lặp lại
    }
}