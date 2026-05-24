#include "coreiot.h"
#include "global.h" // <--- BẮT BUỘC CÓ để dùng biến toàn cục
#include <ArduinoJson.h> // Cần thư viện này để phân tích lệnh từ Server

WiFiClient espClient;
PubSubClient client(espClient);

// Hàm kết nối lại khi mất mạng
void reconnect() {
  // Lặp cho đến khi kết nối được
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Tạo Client ID ngẫu nhiên
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // --- SỬA LỖI 1: XÁC THỰC VỚI THINGSBOARD ---
    // Cú pháp: connect(clientId, username, password)
    // Username BẮT BUỘC là Token (CORE_IOT_TOKEN)
    // Password để trống hoặc NULL
    if (client.connect(clientId.c_str(), CORE_IOT_TOKEN.c_str(), NULL)) {
        
      Serial.println("connected to CoreIOT Server!");
      
      // Đăng ký nhận lệnh RPC từ Server
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to RPC topic");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Hàm xử lý khi nhận được lệnh từ Server (RPC)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Copy payload vào chuỗi tạm để xử lý
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);

  // Phân tích JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, messageTemp);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // --- SỬA LỖI 3: LOGIC ĐIỀU KHIỂN LED (Rule Chain) ---
  // Giả sử Server gửi method: "setLedMode" với params: 1 (ON) hoặc 0 (OFF)
  // Hoặc method: "setStateLED" với params: "ON" / "OFF" (như code cũ của bạn)
  
  const char* method = doc["method"];
  
  // LOGIC: Map lệnh từ Server vào biến cloudLedState
  int newMode = 0;
  bool isCommandValid = false;

  // Trường hợp 1: Dùng lệnh setLedMode (params là int 0/1)
  if (strcmp(method, "setLedMode") == 0) {
      newMode = doc["params"];
      isCommandValid = true;
  }
  // Trường hợp 2: Dùng lệnh setStateLED (params là string "ON"/"OFF")
  else if (strcmp(method, "setStateLED") == 0) {
      const char* params = doc["params"];
      if (strcmp(params, "ON") == 0) newMode = 1;
      else newMode = 0;
      isCommandValid = true;
  }

  if (isCommandValid) {
      // Dùng Semaphore cập nhật biến toàn cục AN TOÀN
      if (xCloudStateSemaphore != NULL) {
          if (xSemaphoreTake(xCloudStateSemaphore, 100) == pdTRUE) {
              cloudLedState = newMode;
              xSemaphoreGive(xCloudStateSemaphore);
              Serial.printf(">>> Command Update: cloudLedState = %d\n", cloudLedState);
          }
      }
  } else {
      Serial.printf("Unknown method: %s\n", method);
  }
}

void setup_coreiot(){
  // Chờ cho đến khi Task Wifi kết nối thành công (Dùng Semaphore chặn)
  Serial.println("[CoreIoT] Waiting for Internet...");
  while(1){
    // Kiểm tra cờ kết nối Wifi của thầy
    if (isWifiConnected == true) { 
        // Nếu muốn chắc ăn hơn thì chờ SemaphoreInternet
        // if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) { ... }
        break;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }

  Serial.println("\n[CoreIoT] Internet Available! Configuring MQTT...");

  // --- SỬA LỖI 2: DÙNG BIẾN ĐỘNG TỪ FILE INFO.DAT ---
  if (CORE_IOT_SERVER.length() > 0) {
      client.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
      client.setCallback(callback);
      Serial.printf("Server: %s, Port: %s\n", CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.c_str());
  } else {
      Serial.println("[Error] No IoT Info loaded!");
  }
}

void coreiot_task(void *pvParameters){
    
    setup_coreiot();

    while(1){
        // Kiểm tra kết nối MQTT
        if (!client.connected()) {
            reconnect();
        }
        client.loop(); // Cực quan trọng để nhận lệnh RPC

        // Gửi dữ liệu (Telemetry) mỗi 2 giây
        // Chỉ gửi khi có kết nối
        if (client.connected()) {
             // Tạo chuỗi JSON thủ công (nhanh gọn)
             // Lưu ý: Dùng glob_temperature của thầy
             String payload = "{\"temperature\":" + String(glob_temperature) + 
                              ",\"humidity\":" + String(glob_humidity) + 
                              ",\"ledState\":" + String(cloudLedState) + "}";
             
             client.publish("v1/devices/me/telemetry", payload.c_str());
             Serial.println("Published: " + payload);
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS); // Gửi mỗi 2 giây (đừng gửi nhanh quá server chặn)
    }
}