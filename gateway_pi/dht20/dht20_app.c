#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/dht20"

int main() {
    int fd;
    char buffer[128];
    int ret;

    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    printf("Device opened successfully!\n");

    while (1) {
        lseek(fd, 0, SEEK_SET);

        ret = read(fd, buffer, sizeof(buffer) - 1);
        if (ret < 0) {
            perror("Failed to read from device");
            break;
        }

        buffer[ret] = '\0';

        printf("Data from DHT20: %s", buffer);

        sleep(3);
    }

    close(fd);
    printf("Device closed\n");

    return 0;
}