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
- Baseline `platformio.ini`:
-  ```ini
-  [platformio]
-  default_envs = native
-
-  [env:esp32c6]
-  platform = espressif32
-  board = esp32c6-devkitc-1
-  framework = arduino
-  build_flags = -std=gnu++17
-
-  [env:native]
-  platform = native
-  build_flags = -std=gnu++17
-  ```
-  Add `ArduinoJson`, `AsyncTCP`, and `ESP Async WebServer` to the `esp32c6` environment `lib_deps`. The `native` environment exists for host-side builds/tests without hardware.
- Build: `pio run`; flash: `pio run -t upload`; upload LittleFS assets: `pio run -t uploadfs`; tests: `pio test`.
- Copy `data-template/` to `data/` on first setup and keep the working `data/` out of version control (store run-time configs and web assets there).
- Development conventions, module templates, and commit guidelines live in `docs/dev.md` and `AGENTS.md`.

For a deeper dive into module responsibilities and the service lifecycle, start with `docs/README.md` and the individual module documents in `docs/modules/`.
