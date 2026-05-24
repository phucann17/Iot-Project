#include "task_check_info.h"

/**
 * @brief Loads Wi-Fi and IoT configuration data from the "/info.dat" file on LittleFS.
 * If the file exists and contains valid JSON, it populates global configuration variables.
 * Uses strdup() to dynamically allocate C-style strings from JSON values.
 */
void Load_info_File()
{
  // Open the configuration file in read-only mode ("r")
  File file = LittleFS.open("/info.dat", "r");
  
  // If the file does not exist or fails to open, exit the function immediately
  if (!file)
  {
    return; 
  }
  
  // Allocate a dynamic JSON document with a buffer size of 4096 bytes
  DynamicJsonDocument doc(4096);
  
  // Deserialize and parse the JSON stream from the file
  DeserializationError error = deserializeJson(doc, file);
  
  // Check if the JSON deserialization was successful
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str()); // Print the specific parsing error
  }
  else
  {
    // Copy configuration values from JSON into global variables (as C-style strings)
    // strdup() automatically measures string length and allocates dynamic memory on the heap
    WIFI_SSID = strdup(doc["WIFI_SSID"]);
    WIFI_PASS = strdup(doc["WIFI_PASS"]);
    CORE_IOT_TOKEN = strdup(doc["CORE_IOT_TOKEN"]);
    CORE_IOT_SERVER = strdup(doc["CORE_IOT_SERVER"]);
    CORE_IOT_PORT = strdup(doc["CORE_IOT_PORT"]);
  }
  
  // Close the file to release system resources
  file.close();
}

/**
 * @brief Deletes the "/info.dat" configuration file if it exists and restarts the ESP device.
 * Typically used to reset stored credentials back to factory defaults.
 */
void Delete_info_File()
{
  // Check if the configuration file exists on the flash file system
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat"); // Delete the file from the system
  }
  
  // Restart the ESP microcontroller to apply changes and enter initial setup state
  ESP.restart();
}

/**
 * @brief Saves Wi-Fi and IoT configurations to the "/info.dat" file on LittleFS as a JSON string.
 * After successfully saving, the device automatically restarts to apply new network settings.
 * * @param wifi_ssid       Wi-Fi SSID (Network Name)
 * @param wifi_pass       Wi-Fi Password
 * @param CORE_IOT_TOKEN  Authentication token for the IoT platform
 * @param CORE_IOT_SERVER IoT server address (IP address or Domain)
 * @param CORE_IOT_PORT   IoT server port number
 */
void Save_info_File(String wifi_ssid, String wifi_pass, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT)
{
  // Print received Wi-Fi credentials to the Serial Monitor for debugging purposes
  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);

  // Initialize a dynamic JSON document with a 4096-byte buffer capacity
  DynamicJsonDocument doc(4096);
  
  // Map input parameters to their respective keys in the JSON object
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  doc["CORE_IOT_TOKEN"] = CORE_IOT_TOKEN;
  doc["CORE_IOT_SERVER"] = CORE_IOT_SERVER;
  doc["CORE_IOT_PORT"] = CORE_IOT_PORT;

  // Open the file in write mode ("w"). This overrides the file if it already exists.
  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    // Serialize the JSON object and write it directly into the configuration file
    serializeJson(doc, configFile);
    configFile.close(); // Close the file stream after writing
  }
  else
  {
    // Log an error message if the file could not be created or opened for writing
    Serial.println(F("Unable to save the configuration."));
  }
  
  // Restart the device to reload the newly saved configuration
  ESP.restart();
}

/**
 * @brief Checks whether valid configuration data exists.
 * If the 'check' flag is false, it initializes LittleFS and loads the config file onto RAM.
 * If both WIFI_SSID and WIFI_PASS are empty, it triggers AP mode to allow user configuration.
 * * @param check  If true, only checks the current state in RAM without re-initializing LittleFS or re-loading the file.
 * @return true  If valid configuration credentials are present.
 * @return false If configuration is missing or filesystem mounting fails (triggers setup/AP mode).
 */
bool check_info_File(bool check)
{
  // If check is false, initialize the hardware filesystem and load configurations
  if (!check)
  {
    // Mount the LittleFS partition. The 'true' argument format the partition if mounting fails initially.
    if (!LittleFS.begin(true))
    {
      Serial.println(" Failed to mount LittleFS!");
      return false; // Return false to indicate a critical filesystem error
    }
    
    // Call function to read the config file from flash memory into global variables
    Load_info_File();
  }
  
  // Core validation: If both the Wi-Fi SSID and Password are empty (not yet configured)
  if (WIFI_SSID.isEmpty() && WIFI_PASS.isEmpty())
  {
    // If this is part of the initial system startup routine (!check), start the Access Point
    if (!check)
    {
      startAP(); // Activate Access Point mode so users can connect and set up the network
    }
    return false; // Configuration is missing or invalid
  }
  
  return true; // Configuration is complete and ready for connection
}