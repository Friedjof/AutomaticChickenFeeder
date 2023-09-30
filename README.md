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

## Usage
When you use the ESP32 in Access Point mode, the ESP32 creates its own WiFi network. The default IP address to access the ESP32's web server is: http://192.168.4.1 You can open your web browser and enter this IP address to access the web services and features provided by the ESP32.

### Pinout
The following table shows the pinout of the ESP32 microcontroller. The pinout of the DS3231 RTC module and the motor control module may vary depending on the manufacturer.

| ESP32 | RTC | Motor   |
| ----- | --- | ------- |
| 21    | SDA | -       |
| 22    | SCL | -       |
| 4     | INT | -       |
| 2     | -   | CONTROL |
| 3.3V  | VCC | VCC     |
| GND   | GND | GND     |

### Flashing
In the release section, you can find the latest binary files for the microcontrollers. You can use the following commands to flash the binary files to the `ESP32` or `ESP8266` microcontroller.

You need to install the esptool first.
```bash
pip install esptool
```
#### firmware.bin, bootloader.bin and partitions.bin (ESP32)
```bash
esptool.py --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```
Change the `--port` parameter to match your system configuration and the path to the binary files.

#### firmware.bin (ESP8266)
```bash
esptool.py --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x10000 firmware.bin
```
Change the `--port` parameter to match your system configuration and the path to the binary files.

#### spiffs.bin
```bash
esptool.py --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x290000 spiffs.bin
```
Change the `--port` parameter to match your system configuration and the path to the binary files.

## Contributions and Collaboration
We welcome contributions and collaboration on this project. If you would like to make improvements, fix bugs, or add new features, please create an issue or a pull request.

### Build project
Change the `BOARD` variable in the Makefile to select the correct board. The default value is `esp32dev`. You can use the following command to build the project.
```bash
make build
```
You can use the following command to flash the project to the microcontroller.
```bash
make flash
```
For the file system, you can use the following commands. You have to move or change the `config.json` file in the `data` folder. Visit the `config.json-template` file for more information.
```bash
make fs
make uploadfs
```

For more advanced commands, you can use the help command.
```bash
make help
```
## Helpful Links
* [PlatformIO and ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html)
* [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
* [DS3231 RTC](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
* [RTC Interrupt](https://github.com/IowaDave/RTC-DS3231-Arduino-Interrupt)
* [RTC Synchronization](https://github.com/Friedjof/SyncRTC)
* [Battery Operation](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
* [ESP32 deep sleep](https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/)

## Author
- [Friedjof Noweck](https://github.com/Friedjof)
- [Bernhard schlagheck](https://github.com/bschlagheck)
