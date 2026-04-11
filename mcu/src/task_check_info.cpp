#include "task_check_info.h"

/**
 * @brief Loads Wi-Fi and IoT configuration data from the "/info.dat" file on LittleFS.
 *        If the file exists and contains valid JSON, it populates global configuration variables.
 *        Uses strdup() to dynamically allocate C-style strings from JSON values.
 */
void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file)
  {
    return; // File not found or cannot be opened
  }
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }
  else
  {
    // Copy configuration values from JSON into global variables (as C strings)
    WIFI_SSID = strdup(doc["WIFI_SSID"]);
    WIFI_PASS = strdup(doc["WIFI_PASS"]);
    CORE_IOT_TOKEN = strdup(doc["CORE_IOT_TOKEN"]);
    CORE_IOT_SERVER = strdup(doc["CORE_IOT_SERVER"]);
    CORE_IOT_PORT = strdup(doc["CORE_IOT_PORT"]);
  }
  file.close();
}

/**
 * @brief Deletes the "/info.dat" configuration file if it exists and restarts the ESP device.
 *        Typically used to reset stored credentials.
 */
void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

/**
 * @brief Saves Wi-Fi and IoT configuration to the "/info.dat" file in LittleFS as JSON.
 *        After saving, the device is restarted to apply the new configuration.
 * 
 * @param wifi_ssid       Wi-Fi SSID (network name)
 * @param wifi_pass       Wi-Fi password
 * @param CORE_IOT_TOKEN  IoT authentication token
 * @param CORE_IOT_SERVER IoT server address
 * @param CORE_IOT_PORT   IoT server port
 */
void Save_info_File(String wifi_ssid, String wifi_pass, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT)
{
  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);

  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  doc["CORE_IOT_TOKEN"] = CORE_IOT_TOKEN;
  doc["CORE_IOT_SERVER"] = CORE_IOT_SERVER;
  doc["CORE_IOT_PORT"] = CORE_IOT_PORT;

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    serializeJson(doc, configFile);
    configFile.close();
  }
  else
  {
    Serial.println(F("Unable to save the configuration."));
  }
  ESP.restart();
}

/**
 * @brief Checks whether valid configuration data exists.
 *        If the 'check' flag is false, it initializes LittleFS and loads the config file.
 *        If both WIFI_SSID and WIFI_PASS are empty, it triggers AP mode (if not just checking).
 * 
 * @param check  If true, only checks current state without initializing FS or loading file.
 * @return true  If valid configuration is present.
 * @return false If configuration is missing or invalid (and may enter setup/AP mode).
 */
bool check_info_File(bool check)
{
  if (!check)
  {
    // Initialize LittleFS (format if needed) and load existing config
    if (!LittleFS.begin(true))
    {
      Serial.println("❌ Failed to mount LittleFS!");
      return false;
    }
    Load_info_File();
  }
  
  // If essential Wi-Fi credentials are missing, configuration is considered invalid
  if (WIFI_SSID.isEmpty() && WIFI_PASS.isEmpty())
  {
    if (!check)
    {
      startAP(); // Start Access Point mode for configuration
    }
    return false;
  }
  return true;
}