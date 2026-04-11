# Smart Environment Monitor with TinyML & IoT (ESP32-S3)

## Overview
Hệ thống giám sát môi trường thông minh tích hợp AI (TinyML) chạy trên nền tảng **ESP32-S3** và hệ điều hành thời gian thực **FreeRTOS**. Dự án có khả năng phát hiện bất thường (Anomaly Detection), cảnh báo qua đèn LED/LCD và đồng bộ dữ liệu lên Cloud (ThingsBoard) qua giao thức MQTT.

## Key Features
* **Multi-tasking RTOS:** Sử dụng FreeRTOS để quản lý 5 tác vụ song song (Sensor, AI, LED, WiFi, IoT).
* **TinyML Integration:** Tích hợp mô hình TensorFlow Lite để phát hiện bất thường nhiệt độ/độ ẩm.
* **IoT Cloud Dashboard:** Kết nối ThingsBoard để vẽ biểu đồ và điều khiển thiết bị từ xa (RPC Remote Control).
* **Custom Web Server:** Giao diện Web Dashboard (Access Point Mode) cho phép cấu hình thiết bị trực tiếp.
* **Smart Synchronization:** Sử dụng Binary Semaphore để đảm bảo an toàn dữ liệu giữa các luồng (Thread-safe).

## Tech Stack
* **Hardware:** ESP32-S3 (Yolo UNO), DHT20 Sensor, LCD I2C, NeoPixel.
* **Firmware:** C/C++ (PlatformIO Framework).
* **Libraries:** TensorFlowLite_ESP32, ThingsBoard, Adafruit_NeoPixel, ESPAsyncWebServer.
* **Protocols:** MQTT, HTTP, I2C, Modbus (Optional).

## Demo
[Chèn link video YouTube demo của bạn vào đây]

## Setup & Installation
1. Clone the repository.
2. Open with VS Code + PlatformIO.
3. Update WiFi & Token credentials in src/global.cpp.
4. Upload Filesystem Image (for Web UI).
5. Upload Firmware.
# Espressif 32: development platform for [PlatformIO](https://platformio.org)

[![Build Status](https://github.com/platformio/platform-espressif32/workflows/Examples/badge.svg)](https://github.com/platformio/platform-espressif32/actions)

ESP32 is a series of low-cost, low-power system on a chip microcontrollers with integrated Wi-Fi and Bluetooth. ESP32 integrates an antenna switch, RF balun, power amplifier, low-noise receive amplifier, filters, and power management modules.

* [Home](https://registry.platformio.org/platforms/platformio/espressif32) (home page in the PlatformIO Registry)
* [Documentation](https://docs.platformio.org/page/platforms/espressif32.html) (advanced usage, packages, boards, frameworks, etc.)

# Usage

1. [Install PlatformIO](https://platformio.org)
2. Create PlatformIO project and configure a platform option in [platformio.ini](https://docs.platformio.org/page/projectconf.html) file:

## Stable version

See `platform` [documentation](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform.html#projectconf-env-platform) for details.

```ini
[env:stable]
; recommended to pin to a version, see https://github.com/platformio/platform-espressif32/releases
; platform = espressif32 @ ^6.0.1
platform = espressif32
board = yolo_uno
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

## Development version

```ini
[env:development]
platform = https://github.com/platformio/platform-espressif32.git
board = yolo_uno
framework = arduino
monitor_speed = 115200
build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

    
# Configuration

Please navigate to [documentation](https://docs.platformio.org/page/platforms/espressif32.html).