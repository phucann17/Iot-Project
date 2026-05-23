#include "task_toogle_boot.h"

// Pin definition for the BOOT button (typically connected to GPIO0 on many ESP32/ESP8266 boards)
#define BOOT 0

/**
 * @brief FreeRTOS task that monitors the BOOT button for a long press (≥2 seconds).
 *        If the button is held down long enough, it triggers a factory reset:
 *        - Deletes the stored configuration file ("/info.dat")
 *        - Restarts the device (via Delete_info_File())
 *        - Deletes this task before restart for clean shutdown.
 * 
 * This task runs indefinitely with a 100ms polling interval to detect button press duration.
 * 
 * @param pvParameters Unused (required by FreeRTOS task signature).
 */
void Task_Toogle_BOOT(void *pvParameters)
{
    unsigned long buttonPressStartTime = 0; // Timestamp when button was first pressed

    while (true)
    {
        // Check if BOOT button is currently pressed (active LOW)
        if (digitalRead(BOOT) == LOW)
        {
            // Record the start time on initial press
            if (buttonPressStartTime == 0)
            {
                buttonPressStartTime = millis();
            }
            // If button has been held for more than 2000 ms (2 seconds)
            else if (millis() - buttonPressStartTime > 2000)
            {
                // Trigger factory reset: delete config and restart
                Delete_info_File();  // This function also calls ESP.restart()
                vTaskDelete(NULL);   // Clean up task before restart (optional but safe)
            }
        }
        else
        {
            // Button released — reset timer
            buttonPressStartTime = 0;
        }

        // Poll every 100 ms to balance responsiveness and CPU usage
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}