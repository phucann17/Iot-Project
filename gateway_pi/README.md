# IoT Gateway for Raspberry Pi - Linux Kernel I2C Sensor Drivers & MQTT Application

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Project Structure](#project-structure)
- [Hardware Requirements](#hardware-requirements)
- [System Requirements](#system-requirements)
- [Architecture](#architecture)
- [Features](#features)
- [Installation & Setup](#installation--setup)
- [Building the Project](#building-the-project)
- [Running the Application](#running-the-application)
- [Usage Examples](#usage-examples)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [Development Notes](#development-notes)

---

## 🎯 Project Overview

This IoT Gateway project implements a complete environmental monitoring system for Raspberry Pi that:

1. **Collects sensor data** from two environmental sensors (BH1750 light sensor and DHT20 temperature/humidity sensor) via Linux kernel I2C device drivers
2. **Processes and aggregates** sensor readings in a user-space application
3. **Transmits data** to CoreIOT cloud platform using MQTT protocol
4. **Controls GPIO devices** (LEDs) for status indication and remote control

The project demonstrates:
- Custom Linux kernel I2C character device driver development
- Device tree overlay integration for hardware configuration
- MQTT client implementation for cloud connectivity
- GPIO control through gpiod library
- Embedded C programming on Linux

---

## 📁 Project Structure

```
gateway_pi/
├── README.md                    # This file
├── app/                         # Main MQTT application
│   ├── app                      # Compiled binary
│   ├── app.c                    # Main application source
│   ├── bh1750.ko                # Compiled BH1750 kernel module
│   ├── dht20.ko                 # Compiled DHT20 kernel module
│   └── run.sh                   # Deployment and startup script
├── bh1750/                      # BH1750 ambient light sensor driver
│   ├── bh1750.c                 # Kernel module source
│   ├── bh1750_app.c             # User-space test application
│   ├── bh1750-overlay.dts       # Device tree overlay
│   └── Makefile                 # Build configuration
└── dht20/                       # DHT20 temperature/humidity sensor driver
    ├── dht20.c                  # Kernel module source
    ├── dht20_app.c              # User-space test application
    ├── dht20-overlay.dts        # Device tree overlay
    └── Makefile                 # Build configuration
```

---

## 🔧 Hardware Requirements

### Sensors
- **BH1750FVI**: Digital Ambient Light Sensor
  - I2C Address: 0x23 (default)
  - Power: 3.3V
  - Accuracy: ±20%
  
- **DHT20**: Digital Temperature and Humidity Sensor
  - I2C Address: 0x38
  - Power: 3.3V
  - Temperature Range: -30°C to +80°C
  - Humidity Range: 0-100% RH

### GPIO Devices
- **LED 1**: GPIO 23 (Status indicator)
- **LED 2**: GPIO 24 (Activity indicator)

### Wiring (I2C Bus 1)
```
Raspberry Pi         Sensors
─────────────────────────────────
GPIO 2 (SDA) ────── BH1750/DHT20 SDA
GPIO 3 (SCL) ────── BH1750/DHT20 SCL
3V3 Power    ────── Sensor VCC
GND          ────── Sensor GND
GPIO 23      ────── LED 1 (via 330Ω resistor)
GPIO 24      ────── LED 2 (via 330Ω resistor)
```

---

## 🖥️ System Requirements

### Target Platform
- Raspberry Pi 3/4/Zero W
- Raspbian/Raspberry Pi OS (Debian-based)
- Linux kernel 4.4+

### Required Software
- GCC cross-compiler for ARM (or native on Raspberry Pi)
- Linux kernel headers
- build-essential package
- libmosquitto-dev (for MQTT client library)
- libcjson-dev (for JSON parsing)
- git
- device-tree-compiler (for .dts compilation)

### Installation on Raspberry Pi
```bash
sudo apt-get update
sudo apt-get install -y build-essential linux-headers-$(uname -r)
sudo apt-get install -y libmosquitto-dev libcjson-dev
sudo apt-get install -y device-tree-compiler git
sudo apt-get install -y gpiod libgpiod-dev
```

---

## 🏗️ Architecture

### System Architecture Diagram
```
┌─────────────────────────────────────────────────────────────┐
│                    Raspberry Pi                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │         User Space Application (app.c)              │   │
│  │  ├─ MQTT Client (libmosquitto)                      │   │
│  │  ├─ Sensor Data Reading                             │   │
│  │  ├─ GPIO Control (LEDs)                             │   │
│  │  └─ CoreIOT Integration                             │   │
│  └──────────────┬──────────────────────────────────────┘   │
│                 │                                           │
│  ┌──────────────▼──────────────┐                           │
│  │   File System (Character    │                           │
│  │   Device Drivers)           │                           │
│  │  ├─ /dev/dht20             │                           │
│  │  └─ /dev/BH1750            │                           │
│  └──────────────┬──────────────┘                           │
│                 │                                           │
│  ┌──────────────▼──────────────┐                           │
│  │  Kernel Space (I2C Drivers) │                           │
│  │  ├─ DHT20 Driver Module     │                           │
│  │  └─ BH1750 Driver Module    │                           │
│  └──────────────┬──────────────┘                           │
│                 │                                           │
│  ┌──────────────▼──────────────┐                           │
│  │   Linux I2C Subsystem       │                           │
│  │   & GPIO Subsystem          │                           │
│  └──────────────┬──────────────┘                           │
└─────────────────┼──────────────────────────────────────────┘
                  │
        ┌─────────┴─────────┬─────────┬──────────┐
        │                   │         │          │
    ┌───▼───┐         ┌────▼────┐ ┌──▼───┐  ┌──▼────┐
    │ BH1750│         │  DHT20  │ │LED 1 │  │LED 2  │
    │Sensor │         │ Sensor  │ │GPIO23│  │GPIO24 │
    └───────┘         └─────────┘ └──────┘  └───────┘
```

### I2C Communication Flow
```
User App
  │
  ├─→ open("/dev/dht20")      ──┐
  │   read() DHT20 data          │
  │   close()                    │
  │                              │
  └─→ open("/dev/BH1750")      ──┤──→ Character Device Driver
      read() BH1750 data         │    (file_operations)
      close()                    │
                                 ├──→ I2C Subsystem
                                 │    (i2c_master_send/recv)
                                 │
                                 └──→ Hardware Sensors
                                      (I2C Bus 1)
```

### Driver Architecture

#### BH1750 Light Sensor Driver
```
Module Init
  ├─ Register I2C driver
  ├─ Allocate char device region
  └─ Register char device class
      │
      ├─ Device probe (when sensor detected)
      │  ├─ Initialize I2C client
      │  ├─ Power on sensor
      │  ├─ Set continuous mode
      │  └─ Create /dev/BH1750
      │
      └─ File operations
         ├─ open()  → Allocate buffer
         ├─ read()  → Read lux data, convert to %
         └─ close() → Free buffer
```

#### DHT20 Temperature/Humidity Sensor Driver
```
Module Init
  ├─ Register I2C driver
  ├─ Allocate char device region
  └─ Register char device class
      │
      ├─ Device probe (when sensor detected)
      │  ├─ Initialize I2C client
      │  ├─ Verify calibration
      │  ├─ Start measurement cycle
      │  └─ Create /dev/dht20
      │
      └─ File operations
         ├─ open()  → Allocate buffer
         ├─ read()  → Read temp/humidity, parse
         └─ close() → Free buffer
```

---

## ✨ Features

### 1. Kernel I2C Drivers

#### BH1750 Driver
- ✅ Automatic probe and initialization
- ✅ Power management (ON/RESET)
- ✅ Continuous high-resolution measurement mode
- ✅ Raw lux to percentage conversion
- ✅ Proper I2C error handling
- ✅ Device tree support with overlay

#### DHT20 Driver
- ✅ Sensor calibration verification
- ✅ Automatic measurement triggering
- ✅ 20-bit humidity parsing
- ✅ 20-bit temperature parsing
- ✅ Conversion to human-readable units
- ✅ CRC error checking
- ✅ Device tree support with overlay

### 2. MQTT Application (app.c)
- ✅ Persistent MQTT connection to CoreIOT
- ✅ Periodic sensor data publishing
- ✅ Remote RPC commands for LED control
- ✅ JSON message formatting
- ✅ Automatic reconnection on failure
- ✅ GPIO control using libgpiod
- ✅ Status indication via LEDs

### 3. System Integration
- ✅ Systemd service support (via run.sh)
- ✅ Automatic driver loading at boot
- ✅ Device tree overlay installation
- ✅ Clean module removal
- ✅ Kernel logging and debugging

---

## 🚀 Installation & Setup

### Step 1: Clone or Download the Project

```bash
cd /home/pi
git clone <repository-url> iot-gateway
cd iot-gateway/gateway_pi
```

### Step 2: Install Dependencies

```bash
# Update package list
sudo apt-get update

# Install build tools and libraries
sudo apt-get install -y build-essential
sudo apt-get install -y linux-headers-$(uname -r)
sudo apt-get install -y libmosquitto-dev libcjson-dev
sudo apt-get install -y device-tree-compiler git
sudo apt-get install -y gpiod libgpiod-dev

# Verify installation
arm-linux-gnueabihf-gcc --version
make --version
i2cdetect --version
```

### Step 3: Enable I2C Interface

```bash
# Enable I2C via raspi-config
sudo raspi-config
# Navigate to: Interfacing Options → I2C → Yes

# Verify I2C is enabled
lsmod | grep i2c
# Should see: i2c_bcm2835, i2c_dev

# Check connected devices on I2C bus 1
i2cdetect -y 1
# Should show addresses 0x23 (BH1750) and 0x38 (DHT20)
```

### Step 4: Install Device Tree Overlays

```bash
# Copy overlay files
sudo cp bh1750/bh1750-overlay.dts /boot/overlays/
sudo cp dht20/dht20-overlay.dts /boot/overlays/

# Compile overlays
sudo dtc -I dts -O dtb -o /boot/overlays/bh1750.dtbo bh1750/bh1750-overlay.dts
sudo dtc -I dts -O dtb -o /boot/overlays/dht20.dtbo dht20/dht20-overlay.dts

# Add to /boot/config.txt
echo "dtoverlay=bh1750" | sudo tee -a /boot/config.txt
echo "dtoverlay=dht20" | sudo tee -a /boot/config.txt

# Reboot to apply
sudo reboot
```

---

## 🔨 Building the Project

### Build Individual Drivers

```bash
# Build BH1750 driver
cd bh1750
make
cd ..

# Build DHT20 driver
cd dht20
make
cd ..

# Check compiled modules
ls -la bh1750/bh1750.ko dht20/dht20.ko
```

### Load Kernel Modules

```bash
# Load modules (from app directory)
sudo insmod bh1750/bh1750.ko
sudo insmod dht20/dht20.ko

# Verify loading
lsmod | grep bh1750
lsmod | grep dht20

# Check kernel messages
dmesg | tail -20

# Verify device nodes created
ls -la /dev/bh1750 /dev/dht20
```

### Build the MQTT Application

```bash
# Build the main application
cd app
gcc -o app app.c -lmosquitto -lcjson

# Or with debug flags
gcc -g -O0 -o app app.c -lmosquitto -lcjson -Wall -Wextra

# Verify compilation
file app
ldd app
```

---

## ▶️ Running the Application

### Quick Start

```bash
# Make sure drivers are loaded
sudo insmod app/bh1750.ko
sudo insmod app/dht20.ko

# Give read permissions to device files
sudo chmod 666 /dev/dht20
sudo chmod 666 /dev/BH1750

# Run the application (requires root for GPIO access)
sudo ./app/app
```

### Automated Startup Script

```bash
# Make script executable
chmod +x app/run.sh

# Run with deployment
./app/run.sh

# This script will:
# 1. Check dependencies
# 2. Load kernel modules
# 3. Set device permissions
# 4. Start the application
# 5. Enable GPIO LEDs
# 6. Connect to CoreIOT
```

### As a System Service

```bash
# Create systemd service file
sudo nano /etc/systemd/system/iot-gateway.service
```

Add the following content:
```ini
[Unit]
Description=IoT Gateway MQTT Service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=root
WorkingDirectory=/home/pi/iot-gateway/gateway_pi
ExecStart=/home/pi/iot-gateway/gateway_pi/app/run.sh
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Then enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable iot-gateway.service
sudo systemctl start iot-gateway.service
sudo systemctl status iot-gateway.service

# View logs
sudo journalctl -u iot-gateway.service -f
```

---

## 📊 Usage Examples

### Test Individual Drivers

```bash
# Test BH1750 sensor
cd bh1750
./bh1750_app

# Expected output:
# Light: 45%
# Light: 47%
# Light: 50%

# Test DHT20 sensor
cd ../dht20
./dht20_app

# Expected output:
# Temp=29C Hum=40%
# Temp=30C Hum=42%
```

### Direct Device Reading

```bash
# Read light sensor
cat /dev/BH1750
# Output: 65

# Read temperature/humidity sensor
cat /dev/dht20
# Output: T=25.5 H=60.2

# Continuous monitoring
watch -n 1 "cat /dev/BH1750 && cat /dev/dht20"
```

### Application Output

```bash
sudo ./app/app

# Console output:
# [APP] Initialized GPIO for LEDs
# [APP] GPIO 23 LED initialized
# [APP] GPIO 24 LED initialized
# [APP] Connecting to app.coreiot.io:1883...
# [APP] Connected to MQTT broker
# [APP] Reading sensors...
# [APP] Temp: 28.5°C, Humidity: 55.2%, Light: 75%
# [APP] Publishing to CoreIOT...
# [APP] Message published successfully
# [MQTT] Received RPC: {'method': 'led_blink', 'params': {'led': 1}}
# [APP] Blinking LED 1
```

---

## 📖 API Reference

### Device Files

#### /dev/dht20 - Temperature & Humidity Sensor

**Read Format:**
```
T=<temperature>°C H=<humidity>%
```

**Example:**
```bash
$ cat /dev/dht20
T=25.3 H=58.5
```

**C API:**
```c
int fd = open("/dev/dht20", O_RDONLY);
char buffer[64];
read(fd, buffer, sizeof(buffer));
// buffer contains: "T=25.3 H=58.5"
close(fd);
```

#### /dev/BH1750 - Light Sensor

**Read Format:**
```
<percentage_0_to_100>
```

**Example:**
```bash
$ cat /dev/BH1750
85
```

**C API:**
```c
int fd = open("/dev/BH1750", O_RDONLY);
char buffer[16];
read(fd, buffer, sizeof(buffer));
// buffer contains: "85"
close(fd);
```

### MQTT Topics (CoreIOT)

#### Publishing Topics

- **Telemetry:** `v1/devices/me/telemetry`
  - Message: `{"temperature": 25.3, "humidity": 58.5, "light": 85}`

- **Attributes:** `v1/devices/me/attributes`
  - Message: `{"device_name": "Gateway-Pi-01", "version": "1.0"}`

#### Subscription Topics

- **RPC Requests:** `v1/devices/me/rpc/request/+`
  - Command: `{"method": "led_on", "params": {"led": 1}}`
  - Command: `{"method": "led_off", "params": {"led": 2}}`
  - Command: `{"method": "led_blink", "params": {"led": 1, "count": 3}}`

- **RPC Response:** `v1/devices/me/rpc/response/<request_id>`
  - Message: `{"result": "success", "data": {}}`

### GPIO Control

```c
// Turn LED ON
gpio_write(23, 1);

// Turn LED OFF
gpio_write(23, 0);

// Blink LED
gpio_blink(24);
```

---

## 🔧 Troubleshooting

### Common Issues and Solutions

#### 1. **Device files not found in /dev**

```bash
# Check if drivers are loaded
lsmod | grep bh1750
lsmod | grep dht20

# If not loaded, load them
sudo insmod bh1750/bh1750.ko
sudo insmod dht20/dht20.ko

# Check kernel messages for errors
dmesg | tail -30

# Expected output should show:
# [DHT20] DHT20 driver initialized
# [BH1750] BH1750 driver initialized
# [I2C] 0-0038: new device
# [I2C] 0-0023: new device
```

#### 2. **I2C Detection Failed**

```bash
# Check I2C interface is enabled
lsmod | grep i2c

# Scan I2C bus for devices
i2cdetect -y 1
# Should show 23 (BH1750) and 38 (DHT20)

# If not found, check wiring:
# - GPIO 2 (SDA) → Sensor SDA
# - GPIO 3 (SCL) → Sensor SCL
# - 3V3 → Sensor VCC
# - GND → Sensor GND

# Verify I2C is not disabled
cat /boot/config.txt | grep -i i2c
# Should NOT contain: dtparam=i2c_arm=off
```

#### 3. **Permission Denied Reading Devices**

```bash
# Grant read permissions
sudo chmod 666 /dev/dht20
sudo chmod 666 /dev/BH1750

# Or add user to group
sudo usermod -a -G i2c $USER

# Reboot or log out/in for group changes to take effect
```

#### 4. **MQTT Connection Failed**

```bash
# Check broker connectivity
nc -zv app.coreiot.io 1883

# Check CoreIOT token in app.c
grep "CORE_IOT_TOKEN" app/app.c

# Verify network connection
ping -c 3 app.coreiot.io

# Check firewall rules
sudo iptables -L -n | grep 1883
```

#### 5. **GPIO Access Issues**

```bash
# Check GPIO permissions
ls -la /dev/gpiochip0

# Add user to gpio group
sudo usermod -a -G gpio $USER

# Or run as root
sudo ./app/app

# Verify GPIO library
gpioinfo
```

#### 6. **Build Errors**

```bash
# Missing kernel headers
sudo apt-get install -y linux-headers-$(uname -r)

# Missing libraries
sudo apt-get install -y libmosquitto-dev libcjson-dev

# Clean previous builds
make clean
make distclean

# Rebuild
make
```

#### 7. **Module Load Fails**

```bash
# Check kernel compatibility
uname -r
# Verify headers match kernel version

# Check for conflicting modules
lsmod | grep -i i2c

# View detailed error in dmesg
dmesg | grep -i error | tail -20

# Try loading with verbose
sudo insmod -v bh1750/bh1750.ko
```

---

## 📝 Development Notes

### Sensor Specifications

**BH1750FVI - Ambient Light Sensor**
- Operating Voltage: 3.0V ~ 3.6V
- I2C Address: 0x23 or 0x5C (selectable via ADDR pin)
- Resolution: 0.5 ~ 65535 lux
- Measurement Time: 16ms (typical)
- Mode: Continuous measurement at 1 Hz
- Datasheet: ROHM BH1750FVI-TR

**DHT20 - Temperature & Humidity Sensor**
- Operating Voltage: 2.2V ~ 5.5V
- I2C Address: 0x38
- Temperature Range: -30°C ~ 80°C (±0.3°C accuracy)
- Humidity Range: 0 ~ 100% RH (±2% RH accuracy)
- Measurement Time: 80ms
- Data Format: 3 bytes (humidity + temperature)
- Datasheet: AOSONG DHT20

### Device Tree Configuration

The project uses device tree overlays to automatically instantiate the I2C devices:

```dts
// bh1750-overlay.dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            bh1750@23 {
                compatible = "rohm,bh1750";
                reg = <0x23>;
                status = "okay";
            };
        };
    };
};
```

This overlay:
1. Targets the i2c1 bus (GPIO 2/3)
2. Adds a device node at address 0x23
3. Sets compatible string to match the kernel driver
4. Enables the device automatically at boot

### Driver Module Architecture

Each driver follows the Linux kernel module standard:

```c
// Initialization
module_init()
  ├─ register_i2c_driver()
  └─ alloc_chrdev_region()

// Probe function (called when device detected)
probe()
  ├─ i2c_new_client()
  ├─ cdev_add()
  └─ device_create()

// File operations
read()
  ├─ i2c_master_send()  (trigger measurement)
  ├─ i2c_master_recv()  (read data)
  └─ copy_to_user()     (return to userspace)

// Cleanup
module_exit()
  ├─ i2c_del_driver()
  └─ unregister_chrdev_region()
```

### Memory and Performance Considerations

- **Kernel Memory:** ~20KB per driver
- **User Space:** ~10MB for application
- **I2C Bandwidth:** ~100 kbps (standard mode)
- **Data Rate:** 1 sample/second (configurable)
- **CPU Usage:** <5% on single core

### Testing and Debugging

```bash
# Enable kernel module debugging
echo 8 | sudo tee /proc/sys/kernel/printk

# View kernel logs in real-time
sudo journalctl -kf

# Check I2C traffic
i2cdump -y 1 0x23
i2cdump -y 1 0x38

# Profile application
strace -o trace.log ./app/app
perf record ./app/app
perf report

# Memory profiling
valgrind --leak-check=full ./app/app
```

### Extending the Project

To add a new sensor:

1. Create a new driver directory following the existing structure
2. Implement the I2C probe/read functions
3. Create a device tree overlay
4. Modify app.c to read from the new device
5. Update MQTT topics for the new sensor data

---

## 📄 License

This project is provided as-is for educational and development purposes.

---

## 🤝 Support & Contact

For issues, questions, or contributions:
- Check the troubleshooting section first
- Review kernel logs with `dmesg`
- Verify hardware connections
- Consult sensor datasheets

---

**Last Updated:** 2026-05-23  
**Version:** 1.0  
**Target Platform:** Raspberry Pi 3/4/Zero W