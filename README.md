# ESP32-C6 Arduino Blink POC

This repository now showcases a minimal **ESP32-C6 DevKitC-1** project built with PlatformIO and the Arduino framework. The firmware prints "Hello" messages over the USB serial port and toggles the on-board RGB LED (GPIO8) once per second—perfect as a smoke test that the toolchain, board definition and serial link behave.

## Requirements
- PlatformIO Core (`pip install platformio`) or the VS Code extension
- GNU Make (optional – the `Makefile` wraps common PlatformIO commands)
- ESP32-C6 DevKitC-1 connected over USB-C

## Quick Start
```bash
make            # build the firmware (defaults to the esp32-c6-devkitc-1 env)
make upload PORT=/dev/ttyACM0   # flash (adjust PORT to match `pio device list`)
make monitor PORT=/dev/ttyACM0  # open 115200 baud serial monitor
```
You should see output similar to:
```
Hello from AutomaticChickenFeeder firmware!
Heartbeat 0
Heartbeat 1
...
```
and the board’s RGB LED should change state every second.

## Project Structure
- `src/main.cpp` – minimal Arduino sketch (`setup()` + `loop()`)
- `platformio.ini` – PlatformIO configuration pointing at Tasmota’s ESP32 platform (includes Arduino support for ESP32-C6)
- `.pio-home/` – PlatformIO’s download/cache directory (ignored by git)

Everything else is intentionally sparse to keep the proof-of-concept easy to follow.

## Notes
- The custom platform is pulled from [tasmota/platform-espressif32](https://github.com/tasmota/platform-espressif32). Remove the commit pin if you prefer tracking the latest upstream updates.
- The Arduino core prints to USB CDC (`Serial`); the provided `Makefile` monitors that port.
- If PlatformIO complains about a missing Python virtualenv (`.pio-home/penv`), create one with `python3 -m venv .pio-home/penv` once.

Happy blinking!
