#include "tinyml.h" // Chứa mảng byte model
#include "global.h"            // Chứa biến toàn cục & Semaphore

// Các biến toàn cục cho TFLite (Giữ trong namespace để tránh xung đột)
namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    
    // Cấp phát vùng nhớ cho TFLite (8KB - 16KB là đủ cho model đơn giản)
    constexpr int kTensorArenaSize = 16 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
} 

void setupTinyML() {
    Serial.println("[TinyML] Initializing TensorFlow Lite...");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // 1. Load Model từ file header dht_anomaly_model.h
    model = tflite::GetModel(dht_anomaly_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[TinyML] Error: Model Version Mismatch!");
        return;
    }

    // 2. Nạp các toán tử (Operations) cần thiết
    // AllOpsResolver nạp tất cả, hơi tốn bộ nhớ nhưng tiện lợi
    static tflite::AllOpsResolver resolver;

    // 3. Khởi tạo Interpreter (Bộ dịch)
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // 4. Cấp phát bộ nhớ cho Tensors
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[TinyML] Error: AllocateTensors Failed!");
        return;
    }

    // 5. Trỏ đến vùng Input và Output của model
    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("[TinyML] Setup Done! Ready to run.");
}

void tiny_ml_task(void *pvParameters) {
    // Khởi tạo một lần đầu tiên
    setupTinyML();
    SensorData_t data;
    float t = 0;
    float h = 0;
    while (1) {


        // --- BƯỚC 1: Lấy dữ liệu AN TOÀN (Dùng Semaphore) ---
        // Phải chờ lấy được khóa xSensorStateSemaphore mới được đọc biến chung
        if (xSensorSem != NULL) {
            if (xSemaphoreTake(xSensorSem, 100) == pdTRUE) {
                if (xQueueReceive(xQueueTinyML, &data, 0) == pdTRUE) {
                    t = data.temp; // Lấy nhiệt độ
                    h = data.humi;    // Lấy độ ẩm
                    xSemaphoreGive(xSensorSem);
                }
              
                xSemaphoreGive(xSensorSem); // Trả khóa ngay
            }
        }

        // Chỉ chạy AI khi dữ liệu hợp lệ (tránh NaN hoặc số 0 lúc mới khởi động)
        // Model AI sẽ chạy sai nếu đầu vào là rác
        if (!isnan(t) && !isnan(h) && (t != 0.0f || h != 0.0f)) {
            
            // --- BƯỚC 2: Chạy mô hình AI (Inference) ---
            
            // Gán dữ liệu vào Input Tensor (Model này yêu cầu 2 tham số: Temp, Humi)
            input->data.f[0] = t;
            input->data.f[1] = h;

            // Thực hiện suy luận
            if (interpreter->Invoke() == kTfLiteOk) {
                // Lấy kết quả đầu ra (Anomaly Score: 0.0 -> 1.0)
                float score = output->data.f[0];
                
                // --- BƯỚC 3: Cập nhật kết quả ra biến toàn cục ---
                // Để Task LCD và Task WebServer hiển thị
                // glob_ml_result = score;

                // Logic phát hiện bất thường (Threshold = 0.5)
                // Nếu điểm > 0.5 coi như là BẤT THƯỜNG
                if (score > 0.5) { 
                    glob_is_anomaly = true;
                    Serial.printf(">>> [TinyML] ANOMALY DETECTED! Score: %.2f (T:%.1f H:%.1f)\n", score, t, h);
                } else {
                    glob_is_anomaly = false;
                    // Serial.printf("[TinyML] Normal. Score: %.2f\n", score);
                }
            } else {
                Serial.println("[TinyML] Invoke Failed!");
            }
        }

        // Chạy định kỳ 3 giây/lần (Không cần quá nhanh để tiết kiệm CPU)
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}