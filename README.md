# DHT20 Linux Device Driver

## 1️⃣ Project Directory Structure


```text
dht20-driver/
│
├── Makefile              # Kernel module build instructions
├── dht20.c               # Linux device driver source code
├── dht20-overlay.dts     # Device Tree Overlay for DHT20
├── README.md             # This documentation
└── LICENSE               # GPL license
```

### Description of Each File

- **dht20.c**: Implementation of the Linux character device driver for DHT20.  
- **Makefile**: Used to compile the driver into a kernel module (`.ko`).  
- **dht20-overlay.dts**: Device Tree Overlay declaring the DHT20 sensor on I2C bus (`i2c1`) at address `0x38`.  
- **LICENSE**: GPL license for the driver.  
- **README.md**: Documentation describing project structure, build process, and driver logic.

---

## 2️⃣ Linux Device Driver Development Process

### 2.1 Identify Device Type
- DHT20 is an **I2C sensor**.  
- The driver is an **I2C client driver**, exposing a **character device interface** (`/dev/DHT20`) to user-space.  
- User-space programs can read **temperature** and **humidity** via standard file operations (`open`, `read`).

### 2.2 Character Device Interface

Linux character devices require these **file operations**:

| Function       | Purpose                                                |
|----------------|--------------------------------------------------------|
| `open()`       | Called when user-space opens `/dev/DHT20`. Initializes device access. |
| `release()`    | Called when device is closed. Cleanup or logging.    |
| `read()`       | Reads sensor data and returns it to user-space.      |
| `write()`      | Optional. Not used for DHT20 but can send data if needed. |
| `ioctl()`      | Optional control commands. Currently a placeholder returning 0. |

These are declared in a `file_operations` struct:

```c
static const struct file_operations dht_fops = {
    .owner = THIS_MODULE,
    .open = dht20_open,
    .release = dht20_release,
    .read = dht20_read,
    .write = dht20_write,
    .unlocked_ioctl = dht20_ioctl
};
```
The kernel uses this structure to know which function to call when /dev/DHT20 is accessed.

### 2.3 I2C Client Driver

Linux uses a **client-driver model** for I2C devices:

- **`probe()`**: Called when the driver matches a device on the bus.  
  + Allocates and initializes `struct dht20_data`.  
  + Registers a character device (`alloc_chrdev_region`, `cdev_init`, `cdev_add`).  
  + Creates a device class (`class_create`) and `/dev/DHT20` node.  

- **`remove()`**: Called when the driver is removed.  
  + Frees memory and removes the character device, class, and node.  

- **Key struct:** `struct dht20_data`  
  + Contains `i2c_client *client`, `cdev`, `class`, `device`, and kernel buffer.  
  + Stored in `file->private_data` for use in `read()` and other file operations.

### 2.4 Communicating with DHT20
Steps to read data from the sensor:

- **Trigger measurement (`dht20_trigger_measurement`)**  
  + Sends an I2C command to the DHT20 sensor.  

- **Read raw data (`dht20_read_data`)**  
  + Receives 7 bytes of raw sensor data via I2C.  

- **Parse data (`dht20_parse`)**  
  + Converts 20-bit raw values into temperature (°C) and humidity (%).  

- **Read function (`dht20_read`)**  
  + Combines the above steps.  
  + Waits ~80 ms for the measurement to complete.  
  + Formats the output string and copies it to user-space using `copy_to_user`.  

- **Key structs used:**  
  + `struct dht20_data` – holds `i2c_client`, `cdev`, `class`, `device`, and kernel buffer.  
  + `struct i2c_client *client` – I2C handle.  
  + `char __user *` – user-space buffer pointer. 

### 2.5 Driver Initialization & Exit

- **`dht20_init`**  
  + Registers the I2C driver when the module is loaded.  

- **`dht20_exit`**  
  + Unregisters the driver when the module is removed.  

- **Module information:**  
  + Uses **GPL license**.  
  + Declares author and version.

### 2.6 Device Tree Overlay
- Declares a `"dht20"` device at address **0x38** on **i2c1** bus.  
- Loading the overlay automatically calls the driver’s `probe()` function.

### 2.7 Makefile & Compilation
- Makefile uses KDIR=/lib/modules/$(uname -r)/build to build dht20.ko.
- Common targets in the Makefile:
	+ make all – builds the kernel module (dht20.ko).
	+ make clean – removes compiled files (*.ko, *.o, *.mod.*) from the project directory.
- Compilation & installation workflow:
	+ Build the kernel module:
	```bash
  	make all
- Compile Device Tree Overlay:
	```bash
 	sudo dtc -@ -I dts -O dtb -o dht20.dtbo dht20-overlay.dts
- Install overlay to /boot/overlays:
  	```bash
  	sudo cp dht20.dtbo /boot/overlays/
- Reboot to apply the overlay:
  	```bash
  	sudo reboot
- Insert the driver module:
  	```bash
  	sudo insmod dht20.ko
- Test the device:
  	```bash
  	cat /dev/DHT20
-> Output example: Temp=25C Hum=55%
- Remove the module when done:
  	```bash
	sudo rmmod dht20.ko
Notes:
make all automatically uses the kernel build system to compile the driver against your current kernel.

make clean is useful if you want to rebuild the module from scratch.

Device Tree overlay must be compiled and copied to /boot/overlays before rebooting to ensure the kernel detects the sensor automatically.

### 2.8 Summary of Key Functions & Structs

| Function | Purpose | Steps / Structs used |
|----------|---------|--------------------|
| dht20_probe | Initialize driver and create char device | Uses `struct dht20_data`, `i2c_client`, `cdev`, `class`, `device` |
| dht20_remove | Cleanup driver | Deletes char device, class, frees `struct dht20_data` |
| dht20_open | Open device | Accesses `private_data` |
| dht20_release | Close device | Cleanup |
| dht20_read | Read temperature & humidity | Calls `trigger_measurement` → `read_data` → `parse` → copy to user-space |
| dht20_trigger_measurement | Send measurement command | Uses `i2c_master_send` |
| dht20_read_data | Receive raw data | Uses `i2c_master_recv` |
| dht20_parse | Convert raw values to temperature/humidity | Calculates from 20-bit raw data |

**Key structs:**

- `struct dht20_data` – holds `i2c_client`, `cdev`, `class`, `device`, and buffer  
- `struct i2c_client *client` – I2C handle  
- `char __user *` – user-space buffer pointer
