# Linux I2C Sensor Drivers (BH1750 + DHT20)

## Overview

This project implements two Linux kernel I2C character device drivers for commonly used environmental sensors:

BH1750: Ambient light sensor (lux measurement)
DHT20: Temperature and humidity sensor

Both drivers expose simple character device interfaces under `/dev`, allowing user-space applications to access sensor data easily using standard file operations (`open`, `read`).

This project is designed for embedded Linux learning and demonstrates:

I2C driver development

Character device registration

Device tree matching

User-space and kernel-space communication

---

## Architecture

### 1. I2C Layer
Each sensor is controlled via the Linux I2C subsystem:
- Device is matched using Device Tree (`compatible` string)
- Kernel calls `probe()` when device is detected
- Communication is done using:
  - `i2c_master_send()`
  - `i2c_master_recv()`

### 2. Character Device Layer
Each driver registers a character device:
- Allocates major/minor number (`alloc_chrdev_region`)
- Initializes `cdev` structure
- Creates device node under `/dev`
- Registers file operations:
  - `open()`
  - `read()`
  - `write()` (optional)

### 3. Data Flow
```
User Space App
    ↓ read()/open()
/dev/bh1750 or /dev/dht20
    ↓ file_operations
Kernel Driver
    ↓ I2C communication
Sensor Hardware
```

---

## Features

### BH1750 Driver
- Power ON / RESET initialization
- Continuous high-resolution mode
- Raw lux conversion to percentage scale
- 2-byte I2C read data handling

### DHT20 Driver
- Sensor calibration check
- Trigger measurement command
- 20-bit humidity and temperature parsing
- Conversion to human-readable values

### Common Features
- Character device interface
- Device tree support
- Kernel logging (`printk`)
- User-space C test applications

---

## Build & Install

1. Build kernel modules
   ```bash
   make
   ```
2. Insert modules
   ```bash
   sudo insmod bh1750.ko
   sudo insmod dht20.ko
   ```
3. Check kernel logs
   ```bash
   dmesg | tail
   ```
4. Remove modules
   ```bash
   sudo rmmod bh1750
   sudo rmmod dht20
   ```

---

## Usage

Check device nodes
```bash
ls /dev/bh1750
ls /dev/dht20
```

Read sensor data
```bash
cat /dev/bh1750
cat /dev/dht20
```

Run test applications
```bash
./bh1750_app
./dht20_app
```

---

## Example Output

### BH1750
```
Light: 45%
Light: 47%
Light: 50%
```

### DHT20
```
Temp=29C Hum=40%
Temp=30C Hum=42%
```

---

## Troubleshooting

### 1. Device not found in /dev
Check if driver is loaded:
```bash
lsmod | grep bh1750
lsmod | grep dht20
```
Check kernel log:
```bash
dmesg | tail
```

---

### 2. Permission denied
```bash
sudo chmod 666 /dev/bh1750
sudo chmod 666 /dev/dht20
```

---

### 3. I2C communication failed
Check I2C device detection:
```bash
i2cdetect -y 1
```
Verify sensor wiring (VCC, GND, SDA, SCL)

---

### 4. No output or stuck read
Ensure sensor initialization succeeded
Check `probe()` logs in `dmesg`
Verify Device Tree `compatible` string

---

## Notes
This project is for educational purposes in embedded Linux development
Drivers are simplified and not optimized for production use
Proper error handling and concurrency protection can be improved further