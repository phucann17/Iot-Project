#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <mosquitto.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
// ===== LED GPIO =====
#define LED1_GPIO 23
#define LED2_GPIO 24
// ===== DEVICE FILE =====
#define DHT20_DEVICE   "/dev/dht20"
#define BH1750_DEVICE  "/dev/BH1750"
// ===== CORE IOT CONFIG =====
#define CORE_IOT_SERVER "app.coreiot.io"
#define CORE_IOT_PORT   1883
#define CORE_IOT_TOKEN  "WThvABnF8jf3ADz6tVaj"
#define TOPIC "v1/devices/me/rpc/request/+"
// ===== GPIO WRITE USING GPIOD =====
void gpio_write(int gpio, int value){
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "gpioset --mode=exit gpiochip0 %d=%d",gpio, value);
    system(cmd);
}

// ===== GPIO BLINK =====
void gpio_blink(int gpio){
    char cmd_on[128];
    char cmd_off[128];

    snprintf(cmd_on, sizeof(cmd_on), "gpioset --mode=exit gpiochip0 %d=1", gpio);
    snprintf(cmd_off, sizeof(cmd_off), "gpioset --mode=exit gpiochip0 %d=0", gpio);
    system(cmd_on);
    usleep(500000);
    system(cmd_off);
    usleep(500000);
}

// ===== READ DEVICE FUNCTION =====
int read_device(int fd, char *buffer, size_t size){
    int ret;
    lseek(fd, 0, SEEK_SET);
    ret = read(fd, buffer, size - 1);
    if (ret < 0) return -1;
    buffer[ret] = '\0';
    return ret;
}

// ===== PARSE DHT20 DATA =====
// Example:
// Temp=28C Hum=49%
void parse_dht20(const char *buffer, float *temp, float *humi){
    char *t_ptr = strstr(buffer, "Temp=");
    char *h_ptr = strstr(buffer, "Hum=");
    if (t_ptr != NULL) sscanf(t_ptr, "Temp=%f", temp);
    if (h_ptr != NULL) sscanf(h_ptr, "Hum=%f", humi);
}

// ===== PARSE BH1750 DATA =====
// Example:
// 11
void parse_bh1750(const char *buffer, float *lux){
    *lux = atof(buffer);
}

// ===== SEND MQTT TO CORE IOT =====
void send_mqtt(float temp, float humi, float lux){
    char payload[256];
    char cmd[1024];
    // ===== JSON PAYLOAD =====
    snprintf(payload, sizeof(payload), "{\"temperature\": %.2f, " "\"humidity\": %.2f, " "\"lux\": %.2f}",
             temp, humi, lux);
    // ===== MQTT COMMAND =====
    snprintf(cmd, sizeof(cmd), "mosquitto_pub " "-h %s " "-p %d " "-u %s " "-t v1/devices/me/telemetry " "-m '%s'", CORE_IOT_SERVER,
             CORE_IOT_PORT, CORE_IOT_TOKEN, payload);

    printf("Sending MQTT: %s\n", payload);
    int ret = system(cmd);
    if (ret != 0) printf("MQTT publish failed!\n");
    else printf("MQTT publish success!\n");
}

// ===== LED CONTROL =====
void control_led(float temp, float humi){
    // LED1 -> THEO NHIỆT ĐỘ
    if (temp < 20) {// TEMP < 20 -> OFF
        gpio_write(LED1_GPIO, 0);
        printf("[LED1] TEMP NORMAL -> OFF\n");
    } else if (temp >= 20 && temp < 30) {// 20 <= TEMP < 30 -> ON
        gpio_write(LED1_GPIO, 1);
        printf("[LED1] TEMP WARNING -> ON\n");
    } else {// TEMP >= 30 -> BLINK
        printf("[LED1] TEMP DANGER -> BLINK\n");
        gpio_blink(LED1_GPIO);
    }
    // LED2 -> THEO ĐỘ ẨM
    if (humi < 10) {// HUMI < 10 -> OFF
        gpio_write(LED2_GPIO, 0);
        printf("[LED2] HUMI NORMAL -> OFF\n");
    } else if (humi >= 10 && humi < 40) { // 10 <= HUMI < 40 -> ON
        gpio_write(LED2_GPIO, 1);
        printf("[LED2] HUMI WARNING -> ON\n");
    } else {// HUMI >= 40 -> BLINK
        printf("[LED2] HUMI DANGER -> BLINK\n");
        gpio_blink(LED2_GPIO);
    }
}

void handle_rpc(const char *payload){
    cJSON *json = cJSON_Parse(payload);
    if (!json) return;
    cJSON *method = cJSON_GetObjectItem(json, "method");
    if (!method || strcmp(method->valuestring, "CONTROL") != 0) {
        cJSON_Delete(json);
        return;
    }
    cJSON *params = cJSON_GetObjectItem(json, "params");
    cJSON *led1 = cJSON_GetObjectItem(params, "led1");
    cJSON *led2 = cJSON_GetObjectItem(params, "led2");
    printf("[RPC] CONTROL RECEIVED\n");
    // LED1
    if (strcmp(led1->valuestring, "OFF") == 0) gpio_write(LED1_GPIO, 0);
    else if (strcmp(led1->valuestring, "ON") == 0) gpio_write(LED1_GPIO, 1);
    else if (strcmp(led1->valuestring, "BLINK") == 0) gpio_blink(LED1_GPIO);
    // LED2
    if (strcmp(led2->valuestring, "OFF") == 0) gpio_write(LED2_GPIO, 0);
    else if (strcmp(led2->valuestring, "ON") == 0) gpio_write(LED2_GPIO, 1);
    else if (strcmp(led2->valuestring, "BLINK") == 0) gpio_blink(LED2_GPIO);
    cJSON_Delete(json);
}

void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg)
{
    printf("MSG: %s\n", (char*)msg->payload);
    handle_rpc((char*)msg->payload);
}

void on_connect(struct mosquitto *m, void *obj, int rc)
{
    if(rc == 0) {
        printf("Connected!\n");
        mosquitto_subscribe(m, NULL, TOPIC, 1);
    }
}
// ===== MAIN =====
int main(){
    int dht_fd, bh_fd;
    char dht_buffer[128];
    char bh_buffer[128];
    float temp = 0.0f;
    float humi = 0.0f;
    float lux  = 0.0f;
    // ===== INIT LED =====
    gpio_write(LED1_GPIO, 0);
    gpio_write(LED2_GPIO, 0);
    // ===== OPEN DHT20 =====
    dht_fd = open(DHT20_DEVICE, O_RDONLY);
    if (dht_fd < 0) {
        perror("Failed to open DHT20 device");
        return -1;
    }
    printf("DHT20 device opened successfully!\n");
    // ===== OPEN BH1750 =====
    bh_fd = open(BH1750_DEVICE, O_RDONLY);
    if (bh_fd < 0) {
        perror("Failed to open BH1750 device");
        close(dht_fd);
        return -1;
    }

    printf("BH1750 device opened successfully!\n");
        struct mosquitto *m;

    mosquitto_lib_init();

    m = mosquitto_new("sub", true, NULL);

    mosquitto_username_pw_set(m, CORE_IOT_TOKEN, NULL);

    mosquitto_connect_callback_set(m, on_connect);
    mosquitto_message_callback_set(m, on_message);

    mosquitto_connect(m, CORE_IOT_SERVER, CORE_IOT_PORT, 60);

    // mosquitto_loop_forever(m, -1, 1);

    // mosquitto_destroy(m);
    // mosquitto_lib_cleanup();
    // mosquitto_lib_cleanup();
    mosquitto_loop_start(m);

    // ===== LOOP =====
    while (1) {
        // ===== RESET VALUE =====
        temp = 0.0f;
        humi = 0.0f;
        lux  = 0.0f;
        // ===== READ DHT20 =====
        if (read_device(dht_fd, dht_buffer, sizeof(dht_buffer)) > 0) {
            printf("[DHT20 RAW] [%s]\n", dht_buffer);
            parse_dht20(dht_buffer, &temp, &humi);
            printf("[DHT20 PARSED] Temp=%.2f Humi=%.2f\n", temp, humi);
        } else {
            printf("[DHT20] Read failed: %s\n", strerror(errno));
        }
        // ===== READ BH1750 =====
        if (read_device(bh_fd, bh_buffer, sizeof(bh_buffer)) > 0) {
            printf("[BH1750 RAW] [%s]\n", bh_buffer);
            parse_bh1750(bh_buffer, &lux);
            printf("[BH1750 PARSED] Lux=%.2f\n", lux);
        } else {
            printf("[BH1750] Read failed: %s\n", strerror(errno));
        }
        // ===== MQTT RECEIVE =====
        // ===== CONTROL LED =====
        // control_led(temp, humi);
        // ===== SEND MQTT =====
        send_mqtt(temp, humi, lux);
        printf("----------------------------------\n");

        sleep(3);
    }

    close(dht_fd);
    close(bh_fd);

    printf("Devices closed\n");


    return 0;
}