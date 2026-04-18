# IoT Environment Monitoring System

An integrated IoT solution for environmental monitoring using microcontroller and Linux gateway components.

## Project Overview

This project implements a complete IoT environment monitoring system with two main components:

- **MCU Component**: ESP32-based smart sensor node with TinyML anomaly detection
- **Gateway Pi Component**: Raspberry Pi with custom Linux kernel drivers for sensors

The system provides real-time environmental data collection, anomaly detection using machine learning, cloud connectivity, and local web interface for monitoring and control.

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────┐
│   Sensors       │────│   MCU (ESP32)    │────│  Gateway Pi │
│   (DHT20,       │    │   - TinyML       │    │  (Raspberry │
│    BH1750)      │    │   - Web Server   │    │    Pi)      │
└─────────────────┘    │   - WiFi/MQTT    │    └─────────────┘
                       │   - ThingsBoard  │           │
                       └──────────────────┘           │
                              │                       │
                              └───────────────────────┘
                                       │
                            ┌─────────────────────┐
                            │   Cloud Dashboard   │
                            │    (ThingsBoard)    │
                            └─────────────────────┘
```

## Components

### MCU Component (`mcu/`)

**Platform**: ESP32-S3 with FreeRTOS
**Framework**: PlatformIO + Arduino

#### Features
- **Multi-tasking RTOS**: 5 concurrent tasks (Sensor, AI, LED, WiFi, IoT)
- **TinyML Integration**: TensorFlow Lite for temperature/humidity anomaly detection
- **IoT Cloud**: ThingsBoard MQTT connectivity with remote control (RPC)
- **Web Dashboard**: Local access point mode for device configuration
- **Sensor Integration**: DHT20 (temperature/humidity), BH1750 (light)
- **Visual Feedback**: NeoPixel LEDs and LCD display for status/alerts
- **Thread-safe**: Binary semaphores for data synchronization

#### Key Files
- `src/main.cpp`: Main application entry point
- `src/coreiot.cpp`: Core IoT functionality
- `src/temp_humi_monitor.cpp`: Sensor monitoring with TinyML
- `src/task_webserver.cpp`: Web server implementation
- `src/task_wifi.cpp`: WiFi connectivity
- `include/tinyml.h`: TinyML model definitions
- `data/`: Web interface files (HTML/CSS/JS)

#### Libraries Used
- TensorFlowLite_ESP32
- ThingsBoard
- ESPAsyncWebServer
- Adafruit_NeoPixel
- ArduinoJson
- PubSubClient

### Gateway Pi Component (`gateway_pi/`)

**Platform**: Raspberry Pi with Linux kernel
**Language**: C (Kernel modules)

#### Features
- **Custom Kernel Drivers**: I2C character device drivers for sensors
- **Device Tree Support**: Overlay integration for hardware detection
- **User-space Interface**: `/dev` character devices for sensor access
- **Dual Sensor Support**: BH1750 (light) and DHT20 (temp/humidity)

#### Sensor Drivers
- **BH1750 Driver**: Ambient light sensor with lux measurement
- **DHT20 Driver**: High-accuracy temperature and humidity sensor

#### Key Files
- `bh1750/bh1750.c`: BH1750 kernel driver
- `bh1750/bh1750_app.c`: User-space test application
- `bh1750/bh1750-overlay.dts`: Device tree overlay
- `dht20/dht20.c`: DHT20 kernel driver
- `dht20/dht20_app.c`: User-space test application
- `dht20/dht20-overlay.dts`: Device tree overlay

## Hardware Requirements

### MCU
- ESP32-S3 development board (Yolo UNO or ESP32 DevKit)
- DHT20 temperature/humidity sensor
- BH1750 ambient light sensor
- NeoPixel LED strip
- LCD I2C display (optional)
- Power supply (USB or battery)

### Gateway Pi
- Raspberry Pi (any model with I2C)
- DHT20 sensor
- BH1750 sensor
- I2C bus enabled

## Software Setup

### MCU Setup
1. Install PlatformIO in VS Code
2. Open the `mcu/` folder
3. Update WiFi credentials in `src/global.cpp`
4. Configure ThingsBoard token
5. Build and upload firmware:
   ```bash
   pio run -t upload
   ```
6. Upload filesystem (web interface):
   ```bash
   pio run -t uploadfs
   ```

### Gateway Pi Setup
1. Enable I2C on Raspberry Pi:
   ```bash
   sudo raspi-config
   ```
2. Install kernel headers:
   ```bash
   sudo apt update
   sudo apt install raspberrypi-kernel-headers device-tree-compiler
   ```
3. Build and install drivers:
   ```bash
   cd gateway_pi/bh1750
   make
   sudo insmod bh1750.ko

   cd ../dht20
   make
   sudo insmod dht20.ko
   ```
4. Install device tree overlays (optional)

## Usage

### MCU
- **Web Interface**: Connect to ESP32 AP, access `192.168.4.1`
- **Cloud Dashboard**: View data on ThingsBoard
- **Serial Monitor**: Debug via PlatformIO serial monitor
- **LED Indicators**: Visual status feedback

### Gateway Pi
- **Sensor Reading**:
  ```bash
  cat /dev/bh1750  # Light sensor
  cat /dev/dht20   # Temp/humidity sensor
  ```
- **Test Applications**:
  ```bash
  cd gateway_pi/bh1750 && ./bh1750_app
  cd ../dht20 && ./dht20_app
  ```

## Data Flow

1. **Sensor Data Collection**: MCU reads DHT20 and BH1750 sensors
2. **Local Processing**: TinyML model analyzes data for anomalies
3. **Local Alerts**: LED/LCD notifications for detected anomalies
4. **Cloud Upload**: MQTT transmission to ThingsBoard
5. **Remote Monitoring**: Web dashboard for real-time data visualization
6. **Remote Control**: RPC commands from cloud to device

## Configuration

### WiFi Settings
Update in `mcu/src/global.cpp`:
```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
```

### ThingsBoard Configuration
```cpp
const char* THINGSBOARD_SERVER = "demo.thingsboard.io";
const char* TOKEN = "your_device_token";
```

### Device Tree (Gateway Pi)
Ensure overlays are loaded:
```bash
sudo dtoverlay bh1750
sudo dtoverlay dht20
```

## Development

### Building MCU Firmware
```bash
cd mcu
pio run  # Build
pio run -t upload  # Upload
pio device monitor  # Serial monitor
```

### Building Kernel Modules
```bash
cd gateway_pi/bh1750
make clean && make
cd ../dht20
make clean && make
```

### Testing
- **Unit Tests**: PlatformIO test framework
- **Integration Tests**: Manual testing with sensors
- **Cloud Tests**: ThingsBoard connectivity verification

## Troubleshooting

### MCU Issues
- **WiFi Connection**: Check credentials and signal strength
- **MQTT Connection**: Verify ThingsBoard server and token
- **Sensor Reading**: Check I2C wiring and addresses
- **TinyML**: Ensure model is properly trained and loaded

### Gateway Pi Issues
- **Driver Loading**: Check kernel logs with `dmesg`
- **Device Nodes**: Verify `/dev` entries exist
- **Permissions**: Use `sudo` or adjust device permissions
- **I2C Bus**: Test with `i2cdetect -y 1`

## Contributing

1. Fork the repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- ESP32 and PlatformIO communities
- TensorFlow Lite for Microcontrollers
- ThingsBoard IoT platform
- Raspberry Pi Foundation