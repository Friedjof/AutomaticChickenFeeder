# Automatic chicken feeder

## Overview
The automatic chicken feeder is a project developed to automate the feeding of chickens. This system is designed to increase efficiency in poultry farming, optimize feeding, and reduce the daily maintenance effort for poultry care.

![Feeder](images/screenshot_desktop.png)

## Features
- Automated feeding at scheduled times
- Configurable feeding schedules and quantities through a web-based user interface
- Secure storage of feeding data in the microcontroller's permanent memory
- Creates a secure Wi-Fi network for system configuration and monitoring
- Can enter a power-saving mode to extend battery life

## Hardware
- ESP32 microcontroller
- Motor control module (e.g., servo motor)
- DS3231 Real-Time Clock module for accurate timekeeping
- Smartphones, tablets, or computers with a web browser and Wi-Fi
- Power supply (battery or power adapter)

## Installation and Configuration
1. Clone this repository.
2. Rename `data/config.json-template` to `data/config.json` and change the default values to your preferences.
```bash
cp data/config.json-template data/config.json
```
3. Install dependencies (VSCode extension PlatformIO IDE and PlatformIO Core).
4. Configure the `platformio.ini` file to select the correct board and port, or start a Nix shell.
```bash
nix-shell
```

## Makefile
The Makefile provides a set of commands to build, flash, and monitor the microcontroller. You might need to adjust the `PORT` variable in the Makefile to match your system configuration. The default value is `/dev/ttyUSB0`. The following commands are available:
- `make help`: Show help message
- `make shell`: Start a Nix shell
- `make build`: Build firmware
- `make flash`: Flash firmware to microcontroller
- `make monitor`: Monitor serial output
- `make clean`: Clean build files
- `make fs`: Build the SPIFFS file system
- `make uploadfs`: Upload the SPIFFS file system
- `make reupload`: Reupload the SPIFFS file system and open a serial monitor
- `make reload`: build, flash and monitor
- `make start`: build SPIFFS file system, upload SPIFFS file system, build firmware, flash firmware, and open a serial monitor

## Helpful Links
* [PlatformIO and ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html)
* [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
* [DS3231 RTC](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
* [RTC Interrupt](https://github.com/IowaDave/RTC-DS3231-Arduino-Interrupt)
* [RTC Synchronization](https://github.com/Friedjof/SyncRTC)
* [Battery Operation](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
* [ESP32 deep sleep](https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/)

## Usage
When you use the ESP32 in Access Point mode, the ESP32 creates its own WiFi network. The default IP address to access the ESP32's web server is: http://192.168.4.1 You can open your web browser and enter this IP address to access the web services and features provided by the ESP32.

### Pinout
The following table shows the pinout of the ESP32 microcontroller. The pinout of the DS3231 RTC module and the motor control module may vary depending on the manufacturer.
| ESP32 Pin | RTC Pin |
| --------- | ------- |
| 21        | SDA     |
| 22        | SCL     |
| 4         | INT     |
| 3.3V      | VCC     |
| GND       | GND     |

| ESP32 Pin | Motor Pin |
| --------- | --------- |
| 2         | OUT       |
| GND       | GND       |
| 3.3V      | VCC       |

## Contributions and Collaboration
We welcome contributions and collaboration on this project. If you would like to make improvements, fix bugs, or add new features, please create an issue or a pull request.

## Author
- [Friedjof Noweck](https://github.com/Friedjof)
