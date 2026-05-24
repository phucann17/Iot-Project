#include "mainserver.h"
#include "global.h" // Cần file này để lấy glob_temperature, glob_ml_result...
#include <WiFi.h>
#include <WebServer.h>

// --- CẤU HÌNH ---
#define BOOT_PIN 0
#define LED_PIN 48 // LED trên board Yolo UNO

// --- BIẾN TRẠNG THÁI ---
bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;
unsigned long connect_start_ms = 0;
bool connecting = false;

WebServer server(80);

// ============================================================
// 1. GIAO DIỆN DARK MODE (CHUYÊN QUAY DEMO)
// ============================================================
String mainPage()
{
  return R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Monitor</title>
    <style>
      /* DARK THEME STYLE */
      body { font-family: 'Segoe UI', sans-serif; background-color: #121212; color: #fff; text-align: center; margin: 0; padding: 20px; }
      
      .container {
        max-width: 400px; margin: 0 auto; background: #1e1e1e;
        border-radius: 20px; box-shadow: 0 0 20px rgba(0, 255, 255, 0.1);
        overflow: hidden; border: 1px solid #333;
      }

      .header {
        background: linear-gradient(90deg, #00c6ff, #0072ff);
        padding: 20px;
      }
      .header h1 { margin: 0; font-size: 1.5em; text-transform: uppercase; letter-spacing: 2px; }
      .header p { margin: 5px 0 0; font-size: 0.8em; opacity: 0.9; }

      .content { padding: 20px; }

      /* Sensor Cards */
      .card-grid { display: flex; gap: 15px; margin-bottom: 20px; }
      .card { flex: 1; background: #2d2d2d; padding: 15px; border-radius: 15px; box-shadow: inset 0 0 10px rgba(0,0,0,0.5); }
      .label { display: block; font-size: 0.8em; color: #aaa; margin-bottom: 5px; }
      .value { font-size: 1.5em; font-weight: bold; color: #00e5ff; }
      .unit { font-size: 0.8em; color: #666; }

      /* Toggle Switches */
      .control-row { display: flex; justify-content: space-between; align-items: center; background: #252525; padding: 15px; border-radius: 12px; margin-bottom: 10px; border: 1px solid #333; }
      .switch { position: relative; display: inline-block; width: 50px; height: 26px; }
      .switch input { opacity: 0; width: 0; height: 0; }
      .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #444; transition: .4s; border-radius: 34px; }
      .slider:before { position: absolute; content: ""; height: 20px; width: 20px; left: 3px; bottom: 3px; background-color: white; transition: .4s; border-radius: 50%; }
      input:checked + .slider { background-color: #00e676; box-shadow: 0 0 10px #00e676; }
      input:checked + .slider:before { transform: translateX(24px); }

      /* AI Section */
      .ai-section { margin-top: 20px; padding: 15px; background: #252525; border-radius: 12px; border: 1px solid #ff3d00; }
      .ai-header { display: flex; justify-content: space-between; font-weight: bold; color: #ff9e80; margin-bottom: 5px; }
      .bar-bg { background: #444; height: 8px; border-radius: 4px; overflow: hidden; }
      .bar-fill { background: #ff3d00; height: 100%; width: 0%; transition: width 0.5s; box-shadow: 0 0 8px #ff3d00; }
      .ai-msg { font-size: 0.8em; margin-top: 8px; color: #aaa; }

      .btn-settings { display: block; margin-top: 20px; padding: 12px; width: 100%; background: #333; color: #aaa; text-decoration: none; border-radius: 8px; font-weight: bold; border: 1px solid #444; box-sizing: border-box; }
    </style>
  </head>
  <body>
    <div class="container">
      <div class="header">
        <h1>Smart Monitor</h1>
        <p>ESP32 IoT Dashboard</p>
      </div>

      <div class="content">
        <div class="card-grid">
          <div class="card">
            <span class="label">TEMP</span>
            <span class="value" id="temp">--</span> <span class="unit">°C</span>
          </div>
          <div class="card">
            <span class="label">HUMIDITY</span>
            <span class="value" id="hum">--</span> <span class="unit">%</span>
          </div>
        </div>

        <div class="control-row">
          <span>💡 Living Room</span>
          <label class="switch">
            <input type="checkbox" id="led1" onchange="toggle(1)">
            <span class="slider"></span>
          </label>
        </div>
        <div class="control-row">
          <span>🚨 Warning LED</span>
          <label class="switch">
            <input type="checkbox" id="led2" onchange="toggle(2)">
            <span class="slider"></span>
          </label>
        </div>

        <div class="ai-section">
          <div class="ai-header">
            <span>🧠 AI SCORE</span>
            <span id="ai-val">0%</span>
          </div>
          <div class="bar-bg"><div id="ai-bar" class="bar-fill"></div></div>
          <div id="ai-msg" class="ai-msg">System Checking...</div>
        </div>

        <a href="/settings" class="btn-settings">⚙️ CONFIG WIFI</a>
      </div>
    </div>

    <script>
      // Update Data Loop
      setInterval(() => {
        fetch('/sensors')
          .then(res => res.json())
          .then(data => {
            // Sensors
            document.getElementById('temp').innerText = data.temp.toFixed(1);
            document.getElementById('hum').innerText = data.hum.toFixed(1);
            
            // AI Logic
            let score = (data.ai_score * 100).toFixed(0);
            document.getElementById('ai-val').innerText = score + "%";
            document.getElementById('ai-bar').style.width = score + "%";
            
            let msg = document.getElementById('ai-msg');
            if(score > 50) {
                msg.innerText = "⚠️ ANOMALY DETECTED!";
                msg.style.color = "#ff3d00";
            } else {
                msg.innerText = "System Normal";
                msg.style.color = "#00e676";
            }

            // Sync Toggle (Optional)
            // if(data.led1 !== undefined) document.getElementById('led1').checked = (data.led1 == "ON");
          });
      }, 2000);

      // Toggle Logic
      function toggle(id) {
        fetch('/toggle?led=' + id)
          .then(res => res.json())
          .then(json => console.log(json));
      }
    </script>
  </body>
  </html>
  )rawliteral";
}

// ============================================================
// 2. GIAO DIỆN CẤU HÌNH WIFI
// ============================================================
String settingsPage()
{
  return R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wi-Fi Settings</title>
    <style>
      body { font-family: sans-serif; background: #121212; color: #fff; display: flex; justify-content: center; padding-top: 50px; }
      .container { background: #1e1e1e; padding: 30px; border-radius: 15px; width: 300px; text-align: center; border: 1px solid #333; }
      input { width: 100%; padding: 10px; margin: 10px 0; border: none; border-radius: 5px; box-sizing: border-box; }
      button { width: 100%; padding: 10px; background: #0072ff; color: white; border: none; border-radius: 5px; font-weight: bold; cursor: pointer; margin-top: 10px; }
      .back { display: block; margin-top: 15px; color: #aaa; text-decoration: none; }
    </style>
  </head>
  <body>
    <div class="container">
      <h2>📡 WiFi Setup</h2>
      <form method="POST" action="/connect">
        <input name="ssid" type="text" placeholder="WiFi SSID Name" required>
        <input name="pass" type="password" placeholder="WiFi Password">
        <button type="submit">SAVE & CONNECT</button>
      </form>
      <a href="/" class="back">← Back to Dashboard</a>
    </div>
  </body>
  </html>
  )rawliteral";
}

// ============================================================
// 3. HANDLERS (XỬ LÝ API)
// ============================================================

void handleRoot() { server.send(200, "text/html", mainPage()); }
void handleSettings() { server.send(200, "text/html", settingsPage()); }

void handleToggle()
{
  if (server.hasArg("led")) {
    int id = server.arg("led").toInt();
    if (id == 1) {
      led1_state = !led1_state;
      digitalWrite(LED_PIN, led1_state); // Điều khiển LED thật
    }
    else if (id == 2) {
      led2_state = !led2_state;
    }
  }
  String json = "{\"led1\":\"" + String(led1_state ? "ON" : "OFF") + "\",\"led2\":\"" + String(led2_state ? "ON" : "OFF") + "\"}";
  server.send(200, "application/json", json);
}

void handleSensors()
{
  float t = glob_temperature;
  float h = glob_humidity;
  float ai = glob_ml_result; // Lấy kết quả AI

  String json = "{\"temp\":" + String(t) + 
                ",\"hum\":" + String(h) + 
                ",\"ai_score\":" + String(ai) + 
                ",\"led1\":\"" + String(led1_state ? "ON" : "OFF") + "\"}";
  server.send(200, "application/json", json);
}

void handleConnect()
{
  if (server.hasArg("ssid")) {
      WIFI_SSID = server.arg("ssid");
      String p = server.arg("pass");
      WIFI_PASS = p; // Lưu vào biến toàn cục
      
      server.send(200, "text/html", "<h1>Connecting...</h1><p>Please wait 15s. Device will switch to Station Mode.</p>");
      
      // Bắt đầu quy trình kết nối
      isAPMode = false;
      connecting = true;
      connect_start_ms = millis();
      
      // Chỉ gọi begin, KHÔNG dùng vòng lặp while chặn ở đây
      WiFi.mode(WIFI_STA);
      if (WIFI_PASS.isEmpty()) WiFi.begin(WIFI_SSID.c_str());
      else WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
      
      Serial.println("[WiFi] Starting connection...");
  } else {
      server.send(400, "text/plain", "Missing SSID");
  }
}

// ============================================================
// 4. MAIN TASK & SETUP
// ============================================================

void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_POST, handleConnect); // Đổi thành POST cho chuẩn
  server.begin();
}

void startAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Monitor", "12345678"); // Tên Wifi phát ra
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  isAPMode = true;
  connecting = false;
}

void main_server_task(void *pvParameters)
{
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  startAP();
  setupServer();

  while (1)
  {
    server.handleClient();

    // --- LOGIC NÚT BOOT (Chuyển Mode) ---
    if (digitalRead(BOOT_PIN) == LOW)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS); // Debounce
      if (digitalRead(BOOT_PIN) == LOW)
      {
        if (!isAPMode) {
          startAP(); // Mất mạng -> Về AP
        } else {
          // Đang ở AP -> Thử kết nối lại
          isAPMode = false;
          connecting = true;
          connect_start_ms = millis();
          WiFi.mode(WIFI_STA);
          WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
        }
        // Chờ nhả nút
        while(digitalRead(BOOT_PIN) == LOW) vTaskDelay(10);
      }
    }

    // --- LOGIC KIỂM TRA WIFI (Non-blocking) ---
    if (connecting)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
        isWifiConnected = true; 
        
        // Báo hiệu có mạng
        if(xBinarySemaphoreInternet != NULL) xSemaphoreGive(xBinarySemaphoreInternet);

        connecting = false;
      }
      else if (millis() - connect_start_ms > 15000) // Timeout 15s
      { 
        Serial.println("WiFi timeout! Back to AP.");
        startAP();
        isWifiConnected = false;
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // Nhường CPU cho các Task khác
  }
}