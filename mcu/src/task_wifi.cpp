#include "task_wifi.h"

// void startAP()
//     WiFi.mode(WIFI_AP);
//     WiFi.softAP(String("Trung"), String("12345678"));
//     Serial.print("AP IP: ");
//     Serial.println(WiFi.softAPIP());
// }

void startAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Monitor", "12345678"); // Tên Wifi phát ra
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  isAPMode = true;
  connecting = false;
}

/**
 * @brief Connects to a Wi-Fi network in Station (STA) mode using global credentials.
 *        If WIFI_SSID is empty, the current FreeRTOS task deletes itself.
 *        If WIFI_PASS is empty, it connects to an open network.
 *        Once connected, it gives a binary semaphore to signal internet availability.
 */
void startSTA()
{
    if (WIFI_SSID.isEmpty())
    {
        vTaskDelete(NULL);
    }

    WiFi.mode(WIFI_STA);

    if (WIFI_PASS.isEmpty())
    {
        WiFi.begin(WIFI_SSID.c_str());
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("\n✅ WiFi connected!");
    Serial.print("📍 IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("📶 RSSI: ");
    Serial.println(WiFi.RSSI());
    //Give a semaphore here
    xSemaphoreGive(xBinarySemaphoreInternet);
}

/**
 * @brief Checks current Wi-Fi status and reconnects if needed.
 *        Returns true if already connected; otherwise, triggers reconnection via startSTA().
 *        Note: startSTA() blocks until connected or deletes the task if SSID is missing.
 * 
 * @return true if Wi-Fi is connected, false otherwise.
 */
bool Wifi_reconnect()
{
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        return true;
    }
    startSTA();
    return false;
}