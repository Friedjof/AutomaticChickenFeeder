# Developer Notes

- **Toolchain**: PlatformIO + Arduino core for ESP32-C3 using standard espressif32 platform.
- **Hardware**: Seeed Xiao ESP32-C3 development board with 2 servos and button input.
- **Build Targets**: Enhanced Makefile with device port management:
  - `make` / `make build` – compile firmware for ESP32-C3.
  - `make flash` – flash with auto-detected port.
  - `make flash 1` – flash to specific port /dev/ttyACM1.
  - `make monitor` – serial monitor at 115200 baud (auto-detect port).
  - `make monitor 2` – monitor specific port /dev/ttyACM2.
  - `make run` – flash + monitor in one command (auto-detect).
  - `make run 1` – flash + monitor on /dev/ttyACM1.
  - `make list` – list available ESP32 devices with port numbers.
  - `make clean` – remove build artifacts.
- **Dependencies**: 
  - ESP32Servo library (v3.0.9+) for servo motor control
  - Button2 library (v2.5.0+) for advanced button handling
- **Pin Configuration**:
  - Servo 1: GPIO 3 (SERVO1_PIN)
  - Servo 2: GPIO 2 (SERVO2_PIN)
  - Button: Check ButtonService.hpp for pin definition
- **First Run**: If PlatformIO complains about missing Python in `.pio-home/penv`, create it with `python3 -m venv .pio-home/penv` once.

## Code Architecture

- **ButtonService**: Wraps Button2 library, handles single/double/long click events
- **FeedingService**: Controls dual servo system for feed dispensing mechanism
- **WebService**: Placeholder class for future web interface implementation
- **Main Loop**: Simple event-driven architecture with button → servo response

## Current Functionality

The project implements a basic chicken feeder with button-controlled servo operation:
1. Button press triggers feeding cycle
2. Two servos open synchronously (180° and 0° respectively) 
3. 1-second delay for feed dispensing
4. Servos close back to starting positions

This is a functional prototype, not a minimal "hello world" - it provides real chicken feeding capability.
