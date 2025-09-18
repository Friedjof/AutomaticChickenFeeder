# Automatic Chicken Feeder (PlatformIO Edition)

This repository hosts the next-generation firmware for the Automatic Chicken Feeder, rebuilt around PlatformIO for an ESP32-C6 target using the Arduino framework. The codebase focuses on reliability, low-power operation, and modularity so future hardware features—additional sensors, multiple servos, extended web controls—can be added with minimal impact.

## What the Firmware Does
- Runs a Web-based captive portal for configuring up to five recurring feed schedules.
- Stores schedules, clock sync policy, and runtime snapshots in LittleFS/NVS, guarding flash wear.
- Uses a DS3231 RTC alarm to wake the ESP32-C6, trigger a feed cycle, and immediately re-arm the next alarm.
- Controls two servos (dispenser + gate) via a Feeding module that enforces safety timeouts and updates telemetry counters.
- Minimises power draw by disabling WiFi and servo rails when idle, then entering deep sleep until the next event or button press.
- Provides two physical buttons: a **Feed** button for dispensing a single scoop and a **Settings** button that wakes the WiFi SoftAP and opens the captive portal.

## Architecture Snapshot
- **System Controller** orchestrates every service through a common `IService` lifecycle (`init`, `loop`, `onWake`, `onSleep`).
- **Schedule Module** resolves the next due job from user-defined slots (`portion_units`, weekday bitmasks).
- **Clock Module** wraps RTC/I2C operations and owns the single hardware alarm.
- **Storage Module** manages `config.json`, runtime snapshots, schema migrations, and validation.
- **Feeding, Power, WebUI, Input, Notification** modules encapsulate hardware or UI-specific behaviour. See `docs/modules/*.md` for details.

## Working with the Project
- Source code: `src/`, headers in `include/`, docs under `docs/`, tests in `test/`, LittleFS bundle under `data/` (copied from `data-template/`).
- First run: `make setup` (copies `data-template/`, verifies PlatformIO, prefetches dependencies).
- Baseline `platformio.ini`:
  ```ini
  [platformio]
  default_envs = native

  [env:esp32c6]
  platform = espressif32
  board = esp32c6-devkitc-1
  framework = arduino
  build_flags = -std=gnu++17
  lib_deps =
    bblanchon/ArduinoJson
    esphome/AsyncTCP-esphome
    me-no-dev/ESP Async WebServer

  [env:native]
  platform = native
  build_flags = -std=gnu++17
  ```
  Build hardware with `pio run -e esp32c6`; host builds default to the `native` environment.
- Flash: `pio run -t upload -e esp32c6`; upload LittleFS assets: `pio run -t uploadfs -e esp32c6`; tests: `pio test` / `pio test -e esp32c6`.
- Development conventions, module templates, and commit guidelines live in `docs/dev.md` and `AGENTS.md`.

## Development Environment Options
- **VS Code Dev Container** – open the repository in VS Code and choose “Reopen in Container.” The `.devcontainer/` config uses the official PlatformIO Core image and installs common extensions. Works on Windows/macOS/Linux (Docker Desktop or compatible runtime required). Map USB devices in `devcontainer.json` for flashing inside the container.
- **Docker CLI** – run commands without local installation:
  ```bash
  docker run --rm -it \
    -v "$PWD":/workspace -w /workspace \
    platformio/platformio-core pio run -e esp32c6
  ```
  Add `--device=/dev/ttyUSB0` (Linux) or the appropriate device mapping when flashing.
- **Local install** – install PlatformIO Core or the IDE; the provided Makefile targets (`make build`, `make upload`, `make setup`, …) wrap common workflows.

For a deeper dive into module responsibilities and the service lifecycle, start with `docs/README.md` and the individual module documents in `docs/modules/`.
