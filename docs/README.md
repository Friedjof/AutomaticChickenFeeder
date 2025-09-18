# Architecture Overview

## Directory Layout
- `src/main.cpp` bootstraps the firmware; all services live in `src/app/<name>_module.cpp`.
- Headers and shared models sit in `include/app/` and `include/models/`; `include/config/` hosts compile-time pin and board constants.
- `data-template/` contains versioned LittleFS defaults (`config.json`, web bundle). Copy it to an ignored `data/` directory before flashing assets.
- `docs/` stores architecture notes (this file and per-module docs under `docs/modules/`).
- `test/` holds Unity-based unit tests following PlatformIO’s layout.

## PlatformIO Baseline
A typical `platformio.ini` setup:
```ini
[platformio]
default_envs = native

[env:esp32c6]
platform = espressif32
board = esp32c6-devkitc-1
framework = arduino
monitor_speed = 115200
build_flags = -std=gnu++17
lib_deps =
  bblanchon/ArduinoJson
  esphome/AsyncTCP-esphome
  me-no-dev/ESP Async WebServer

[env:native]
platform = native
build_flags = -std=gnu++17
```
Use the `native` environment for host-side builds/tests; target hardware builds should specify `-e esp32c6`. Adjust libraries if you swap the HTTP stack or JSON parser.

## Module Responsibilities
- **System Controller** drives lifecycle (init → loop → sleep → wake) via the shared `IService` interface.
- **Storage Module** is the sole owner of LittleFS/NVS access (`config.json`, runtime snapshots, migrations).
- **Schedule Module** calculates upcoming feed jobs from the fixed five-slot timetable.
- **Clock Module** wraps DS3231 access and keeps exactly one RTC alarm armed.
- **Feeding Module** sequences servo motions for a single scoop per request; handles timeouts/fault events.
- **Power Module** toggles WiFi SoftAP, servo rails, and deep-sleep wake sources.
- **Web UI Module** serves the captive portal backed by assets in `data/`.
- **Input Module** debounces two buttons: *Feed* (dispense one scoop) and *Settings* (enable WiFi/AP).
- **Notification Module** is optional for LEDs/buzzers.

## Execution Flow
1. **Boot/Reset**: detect reset reason, copy `data-template/` → `data/` if missing, load config & snapshot, bring up AP on cold boot.
2. **Main Loop**: iterate `service->loop(ctx)`, drain event queue (RTC alarms, button presses, web commands), dispatch state updates.
3. **Sleep Prep**: call `onSleep()`, persist snapshot, ask Schedule for next job, program RTC alarm, hand control to Power for `esp_deep_sleep_start()`.
4. **Wake**: acknowledge RTC/button cause, run pending feed (Feed button = one scoop), re-arm alarm, optionally enable WiFi if Settings button was pressed.

## Build & Test Workflow
1. Copy `data-template/` to `data/` (kept out of VCS) and drop built Web UI assets there (`npm run build` output).
2. Compile host build: `pio run` (defaults to `native`); build hardware target with `pio run -e esp32c6`. Flash hardware via `pio run -t upload -e esp32c6`; upload assets with `pio run -t uploadfs -e esp32c6`.
3. Tests: `pio test` (host) or `pio test -e esp32c6` when running on hardware. Provide mocks for schedule/clock to cover week wrap, disabled slots, and feed outcomes.

## Open Items
- Event bus & logging helper are still TBD; services should surface TODOs where they need structured logging.
- Document the source location for the Web UI (future `web/` workspace) so `npm run build` has a defined context.
- Finalise the DS3231 driver wrapper (currently assumed via Wire) before hardware validation.
