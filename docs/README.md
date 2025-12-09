# Automatic Chicken Feeder

This project is an **ESP32-C3 based automatic chicken feeder** using PlatformIO. It provides button-controlled servo operation to dispense chicken feed on demand.

## Features

- **Button Control**: Single click to trigger feeding cycle
- **Dual Servo System**: Two servos work in sync to open/close the feeding mechanism
- **Simple Operation**: Press button → servos open → 1 second delay → servos close
- **ESP32-C3**: Uses Seeed Xiao ESP32-C3 development board

## Hardware Requirements

- **Seeed Xiao ESP32-C3** development board
- **2x Servo Motors** (connected to GPIO pins 2 and 3)
- **Push Button** (connected to GPIO pin, defined in ButtonService)
- **Power Supply** suitable for servos

## Build & Flash

The Makefile supports convenient port selection for multiple ESP32 devices:

```bash
make                # build only
make flash          # flash with auto-detected port
make flash 1        # flash to /dev/ttyACM1
make monitor        # monitor with auto-detected port  
make monitor 2      # monitor /dev/ttyACM2
make run            # flash + monitor (auto-detect)
make run 1          # flash + monitor on /dev/ttyACM1
make list           # list available ESP32 devices with port numbers
make clean          # clean build artifacts
```

## Project Structure

- `platformio.ini` – ESP32-C3 configuration with Arduino framework
- `src/main.cpp` – Main application logic with button handling
- `lib/ButtonService/` – Button input handling using Button2 library
- `lib/FeedingService/` – Servo control for feeding mechanism
- `lib/WebService/` – (Placeholder for future web interface)
- `Makefile` – Enhanced build system with device port management

## How It Works

1. **Setup**: ButtonService initializes button with click handler, FeedingService attaches servos
2. **Main Loop**: Continuously checks for button presses
3. **Feed Cycle**: On button click → FeedingService.feed() → servos open (180°) → delay 1s → servos close (0°)
4. **Servo Coordination**: Two servos move in opposite directions for synchronized operation

## Dependencies

- `ESP32Servo` (v3.0.9+) - Servo motor control
- `Button2` (v2.5.0+) - Advanced button handling with debouncing

## Pin Configuration

- **Servo 1**: GPIO 3 (SERVO1_PIN)
- **Servo 2**: GPIO 2 (SERVO2_PIN)  
- **Button**: Defined in ButtonService (check ButtonService.hpp for pin)

## Usage

1. Power on the device
2. Press the button to trigger a feeding cycle
3. Servos will open, wait 1 second, then close
4. Repeat as needed for feeding chickens
