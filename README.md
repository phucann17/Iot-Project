
Linux Device Driver for DHT20 Temperature & Humidity Sensor
1️⃣ Project Directory Structure
dht20-driver/
│
├── Makefile                 # Kernel module build instructions
├── dht20.c                   # Linux device driver source code
├── dht20-overlay.dts         # Device Tree Overlay for DHT20
├── README.md                 # This documentation
└── LICENSE                   # GPL license
Description of Each File:
dht20.c: Contains the implementation of the Linux character device driver for DHT20.
Makefile: Used to compile the driver into a kernel module (.ko).
dht20-overlay.dts: Device Tree Overlay file, declares the DHT20 sensor on the I2C bus (i2c1) at address 0x38.
LICENSE: Provides licensing (GPL) for the driver.
README.md: Documentation describing the structure, build process, and driver logic.
2️⃣ Linux Device Driver Development Process

Developing a Linux device driver involves several steps:

2.1 Identify Device Type
DHT20 is an I2C sensor.
The driver is an I2C client driver, exposing a character device interface (/dev/DHT20) to user-space.
User-space programs can read temperature and humidity via standard file operations (open, read).
2.2 Character Device Interface

Linux character devices require the following file operations:

Function	Purpose
open()	Called when user-space opens /dev/DHT20. Initializes device access.
release()	Called when the device is closed. Cleanup or logging.
read()	Reads sensor data from the hardware and returns it to user-space.
write()	Not used in DHT20, but can send data to the device if needed.
ioctl()	Optional control commands; currently a placeholder returning 0.

These are declared in a file_operations struct:

static const struct file_operations dht_fops = {
    .owner = THIS_MODULE,
    .open = dht20_open,
    .release = dht20_release,
    .read = dht20_read,
    .write = dht20_write,
    .unlocked_ioctl = dht20_ioctl
};
Kernel uses this structure to know which functions to call when user-space accesses /dev/DHT20.
2.3 I2C Client Driver

For I2C devices, Linux uses a client-driver model:

probe(struct i2c_client *client) – called when the driver matches a device on the bus.
remove(struct i2c_client *client) – called when the driver is removed.

In your driver:

static int dht20_probe(struct i2c_client* client) {
    struct dht20_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
    data->client = client;
    i2c_set_clientdata(client, data);
    // Allocate character device and create /dev/DHT20
}
Allocates memory for device data (struct dht20_data).
Registers character device with alloc_chrdev_region + cdev_init + cdev_add.
Creates device class (class_create) and device node (device_create) in /dev.
2.4 Device Data Structure
struct dht20_data {
    struct i2c_client *client;   // I2C handle
    dev_t devt;                  // Major/minor numbers
    struct cdev cdev;            // Character device struct
    struct class *class;         // Device class
    struct device *device;       // Device node in /dev
    uint8_t *buffer;             // Kernel buffer (optional)
};
Holds all information needed by the driver for a device instance.
Stored in the private_data field of struct file for access in read() and other file operations.
2.5 Communicating with DHT20 over I2C
Trigger Measurement:
static int dht20_trigger_measurement(struct i2c_client *client){
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    return i2c_master_send(client, cmd, 3) == 3 ? 0 : -EIO;
}
Sends the measurement command to the sensor.
Read Raw Data:
static int dht20_read_data(struct i2c_client *client, uint8_t *buf){
    return i2c_master_recv(client, buf, 7) == 7 ? 0 : -EIO;
}
Receives 7 bytes of raw sensor data from DHT20.
Parse Data:
static void dht20_parse(uint8_t *data, int *temp, int *hum){
    uint32_t raw_hum  = ((data[1]<<16)|(data[2]<<8)|data[3]) >> 4;
    uint32_t raw_temp = ((data[3]&0x0F)<<16)|(data[4]<<8)|data[5];

    *hum  = (raw_hum * 100) / (1<<20);
    *temp = ((raw_temp * 200) / (1<<20)) - 50;
}
Converts raw 20-bit values to actual temperature (°C) and humidity (%).
2.6 Reading Data in User-Space

The read() function:

static ssize_t dht20_read(struct file* file, char __user *buff, size_t count, loff_t *offset){
    struct dht20_data *data = file->private_data;
    uint8_t raw[7];
    int temp, hum;
    char out[50];

    dht20_trigger_measurement(data->client);
    msleep(80);
    dht20_read_data(data->client, raw);
    dht20_parse(raw, &temp, &hum);

    int len = sprintf(out, "Temp=%dC Hum=%d%%\n", temp, hum);
    copy_to_user(buff, out, len);

    return len;
}
Triggers measurement.
Waits 80 ms for conversion.
Reads raw I2C data.
Converts and formats it to string.
Copies it to user-space buffer.
2.7 Driver Initialization and Exit
static int __init dht20_init(void){
    return i2c_add_driver(&dht20_driver);
}

static void __exit dht20_exit(void){
    i2c_del_driver(&dht20_driver);
}

module_init(dht20_init);
module_exit(dht20_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AN NGUYEN");
MODULE_DESCRIPTION("Linux driver for DHT20 temperature and humidity sensor");
MODULE_VERSION("1.0");
Registers the I2C driver when module is inserted (insmod).
Unregisters driver when module is removed (rmmod).
2.8 Device Tree Overlay

dht20-overlay.dts:

/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";

    fragment@0 {
        target = <&i2c1>;

        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;

            dht20@38 {
                compatible = "dht20";
                reg = <0x38>;
            };
        };
    };
};
Tells the kernel that a device compatible with "dht20" exists at I2C address 0x38 on bus i2c1.
Enables the probe function of the driver to be called automatically.
2.9 Makefile
obj-m += dht20.o

KDIR = /lib/modules/$(shell uname -r)/build

all: 
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
Compiles dht20.c into dht20.ko.
Uses kernel build system (KDIR) for proper module compilation.
2.10 Usage
Compile module:
make
Copy Device Tree Overlay and enable:
sudo dtc -@ -I dts -O dtb -o dht20.dtbo dht20-overlay.dts
sudo cp dht20.dtbo /boot/overlays/
sudo reboot
Insert module:
sudo insmod dht20.ko
Test device:
cat /dev/DHT20
# Output: Temp=25C Hum=55%
Remove module:
sudo rmmod dht20
2.11 Summary of Key Functions & Data Types
Function	Description	Input / Output
dht20_probe	Initializes driver and char device	struct i2c_client*
dht20_remove	Cleans up driver	struct i2c_client*
dht20_open	Opens device, sets private_data	struct inode*, struct file*
dht20_release	Closes device	struct inode*, struct file*
dht20_read	Triggers measurement, reads, parses, returns to user	struct file*, char __user*, size_t, loff_t*
dht20_trigger_measurement	Sends I2C command	struct i2c_client*
dht20_read_data	Receives raw bytes via I2C	struct i2c_client*, uint8_t*
dht20_parse	Converts raw 20-bit values to temp/humidity	uint8_t*, int*, int*

Data Types:

struct i2c_client *client – handle for I2C device.
struct cdev – character device structure.
struct class – device class for /dev.
uint8_t – 8-bit unsigned byte.
int – standard integer (used for temperature/humidity).
char __user * – pointer to user-space buffer.
