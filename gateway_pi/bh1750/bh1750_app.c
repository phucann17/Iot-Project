#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/BH1750"

int main() {
    int fd;
    char buffer[128];
    int ret;

    // open device
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    printf("BH1750 device opened!\n");

    while (1) {
        ret = read(fd, buffer, sizeof(buffer) - 1);
        if (ret < 0) {
            perror("Read failed");
            break;
        }

        buffer[ret] = '\0';

        printf("Light: %s%%\n", buffer);

        sleep(3);
    }

    close(fd);
    printf("Device closed\n");

    return 0;
}