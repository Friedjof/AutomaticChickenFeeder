# Chicken Feeder Automaton

## Overview

The Chicken Feeder Automaton is a project developed to automate the feeding of chickens. This system is designed to increase efficiency in poultry farming, optimize feeding, and reduce the daily maintenance effort for poultry care.

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
- Smartphones, tablets, or computers with a web browser and Wi-Fi
- DS3231 Real-Time Clock module for accurate timekeeping
- Power supply (battery or power adapter)

## Installation and Configuration

1. Clone the repository.
2. Install dependencies (VSCode extension PlatformIO IDE and PlatformIO Core).
3. Configure the `platformio.ini` file to select the correct board and port, or start a Nix shell with `nix-shell`.
4. Use `make help` to display available commands.

## Helpful Links
* [PlatformIO and ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html)
* [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
* [DS3231 RTC](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
* [RTC Interrupt](https://github.com/IowaDave/RTC-DS3231-Arduino-Interrupt)
* [Battery Operation](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
* [ESP32 deep sleep](https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/)

## Contributions and Collaboration

We welcome contributions and collaboration on this project. If you would like to make improvements, fix bugs, or add new features, please create an issue or a pull request.

## Author

- [Friedjof Noweck](https://github.com/Friedjof)
