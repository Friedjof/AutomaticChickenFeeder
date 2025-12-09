# Repository Guidelines

This repository contains an **ESP32-C3 automatic chicken feeder** project using PlatformIO. The goal is to provide a functional feeding system with button control and servo actuation.

## Build & Flash

Always use the enhanced `Makefile` targets for convenient development:

### Basic Commands
- `make` / `make build` – compile firmware for ESP32-C3
- `make clean` – clean build artifacts

### Device Port Management
The Makefile supports automatic port detection and manual port selection:

- `make flash` – flash with auto-detected port
- `make flash 1` – flash to /dev/ttyACM1 
- `make monitor` – serial monitor with auto-detected port
- `make monitor 2` – monitor /dev/ttyACM2
- `make run` – flash + monitor (auto-detect)
- `make run 1` – flash + monitor on /dev/ttyACM1
- `make list` – list available ESP32 devices with port numbers

### Port Selection Examples
```bash
# Auto-detect port (recommended for single device)
make flash
make monitor

# Specific port (useful with multiple ESP32 devices)
make flash 2      # Flash to /dev/ttyACM2
make monitor 2    # Monitor /dev/ttyACM2
make run 1        # Flash and monitor /dev/ttyACM1

# List connected ESP32 devices
make list
```

## Hardware Setup
- **Board**: Seeed Xiao ESP32-C3
- **Servos**: Connected to GPIO 2 and GPIO 3
- **Button**: Pin defined in ButtonService (check ButtonService.hpp)
- **Power**: Ensure adequate power supply for servo operation

## Code Structure
- `src/main.cpp`: Main application with button handling
- `lib/ButtonService/`: Button input management using Button2 library
- `lib/FeedingService/`: Dual servo control for feeding mechanism
- `lib/WebService/`: Placeholder for future web interface

## Platform Configuration
- `platformio.ini` uses standard `espressif32` platform for ESP32-C3
- Board target: `seeed_xiao_esp32c3`
- Upload speed: 460800 baud for fast flashing
- Dependencies: ESP32Servo v3.0.9+ and Button2 v2.5.0+

## Development Guidelines
- Use the Makefile targets instead of direct `pio` commands
- Test servo operation with appropriate power supply
- Ensure `.pio-home/penv` exists (create via `python3 -m venv .pio-home/penv` if needed)
- Keep code modular with separate service classes

## Current Functionality
The system provides:
1. Button-triggered feeding cycles
2. Synchronized dual servo operation
3. Simple open → delay → close feeding sequence
4. Extensible architecture for future features (web interface, scheduling)

Follow these conventions to maintain code quality and enable easy hardware replication.
