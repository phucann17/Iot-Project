#include <task_handler.h>

/**
 * @brief Handles incoming WebSocket messages, parsing them as JSON and performing actions
 *        based on the message type (e.g., device control or configuration update).
 * 
 * The message is expected to be a JSON object with the following structure:
 * {
 *   "page": "device" or "setting",
 *   "value": { ... }
 * }
 * 
 * For "device": controls a GPIO pin (ON/OFF).  
 * For "setting": updates Wi-Fi and IoT configuration and saves it persistently.
 * 
 * @param message The raw WebSocket message string (assumed to be valid UTF-8 JSON).
 */
void handleWebSocketMessage(String message)
{
    Serial.println(message);
    StaticJsonDocument<256> doc;

    // Attempt to parse the incoming message as JSON
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Serial.println("❌ Failed to parse JSON!");
        return;
    }

    // Extract the "value" object (common payload container)
    JsonObject value = doc["value"];

    // Handle device control commands (GPIO toggling)
    if (doc["page"] == "device")
    {
        // Validate required fields
        if (!value.containsKey("gpio") || !value.containsKey("status"))
        {
            Serial.println("⚠️ JSON missing required fields: 'gpio' or 'status'");
            return;
        }

        int gpio = value["gpio"];                     // GPIO pin number
        String status = value["status"].as<String>(); // Expected: "ON" or "OFF"

        Serial.printf("⚙️ Controlling GPIO %d → %s\n", gpio, status.c_str());

        // Configure pin as output and set state
        pinMode(gpio, OUTPUT);
        if (status.equalsIgnoreCase("ON"))
        {
            digitalWrite(gpio, HIGH);
            Serial.printf("🔆 GPIO %d turned ON\n", gpio);
        }
        else if (status.equalsIgnoreCase("OFF"))
        {
            digitalWrite(gpio, LOW);
            Serial.printf("💤 GPIO %d turned OFF\n", gpio);
        }
        // Note: Invalid status values are silently ignored
    }
    // Handle configuration update (Wi-Fi + IoT settings)
    else if (doc["page"] == "setting")
    {
        // Extract new configuration parameters
        String WIFI_SSID = doc["value"]["ssid"].as<String>();
        String WIFI_PASS = doc["value"]["password"].as<String>();
        String CORE_IOT_TOKEN = doc["value"]["token"].as<String>();
        String CORE_IOT_SERVER = doc["value"]["server"].as<String>();
        String CORE_IOT_PORT = doc["value"]["port"].as<String>();

        // Log received settings (avoid logging passwords in production!)
        Serial.println("📥 Received configuration via WebSocket:");
        Serial.println("SSID: " + WIFI_SSID);
        Serial.println("PASS: " + WIFI_PASS);
        Serial.println("TOKEN: " + CORE_IOT_TOKEN);
        Serial.println("SERVER: " + CORE_IOT_SERVER);
        Serial.println("PORT: " + CORE_IOT_PORT);

        // Persistently save the new configuration to LittleFS
        // ⚠️ This function restarts the device after saving!
        Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);

        // Optional: Acknowledge successful save to all connected WebSocket clients
        String msg = "{\"status\":\"ok\",\"page\":\"setting_saved\"}";
        ws.textAll(msg);
    }
    // Note: Messages with unknown "page" values are silently ignored
}