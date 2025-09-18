# Repository Guidelines

## Project Structure & Module Organization
- Firmware lives under `src/`; each service is split into `src/app/<name>_module.cpp` and `include/app/<name>_module.hpp`.
- Shared data models reside in `include/models/`; configuration constants go in `include/config/` (e.g., pin maps).
- `data-template/` holds versioned LittleFS defaults (web bundle, `config.json`). Copy it to an ignored `data/` directory before flashing.
- Documentation sits in `docs/`; unit and integration tests belong in `test/`, following PlatformIO’s standard layout.

## Build, Test & Development Commands
- `make setup` – copy `data-template/` → `data/`, verify PlatformIO CLI, prefetch packages.
- `pio run` – compile the firmware (defaults to the host `native` environment).
- `pio run -e esp32c6` – compile for the ESP32-C6 Arduino environment once platforms/libs are available.
- `pio run -t upload -e esp32c6` – flash the compiled firmware to the device.
- `pio run -t uploadfs -e esp32c6` – upload LittleFS assets from `data/` (copied from `data-template/`).
- `pio test` / `pio test -e esp32c6` – execute Unity tests on host or hardware.
- `npm run build` inside the Web UI workspace (when added) – produce static assets before `uploadfs`.

## Coding Style & Naming Conventions
- Use C++17, `#pragma once` in headers, and 2/4-space indentation consistent with existing files.
- Modules end with `_module.cpp/.hpp` and live in `app::` namespaces (e.g., `app::power::PowerModule`).
- Avoid `using namespace` in headers; prefer explicit namespaces and `constexpr` for config values.
- `ServiceContext` mediates cross-module access—do not instantiate global singletons.

## Testing Guidelines
- Tests rely on PlatformIO’s Unity framework; place cases under `test/<feature>/test_main.cpp`.
- Name tests after the behaviour under scrutiny (e.g., `test_schedule_next_due.cpp`).
- Ensure scheduler/clock logic has edge-case coverage (week wrap, disabled slots); run `pio test` before submitting changes.

## Commit & Pull Request Guidelines
- Follow concise commit messages in the style “feat: ...”, “fix: ...”, mirroring existing history.
- Include doc updates (`docs/`) when behaviour changes; keep commits focused.
- Pull requests should describe changes, note affected modules, link issues where applicable, and mention manual/automated test results.
- Provide screenshots or logs for UI or hardware-facing changes when useful.

## Architecture Notes
- System Controller coordinates services via `IService` interface and `ServiceContext`.
- Only Storage touches LittleFS/NVS; Clock wraps RTC hardware; Power manages deep sleep/WiFi lifecycle.
- Two buttons are expected: *Feed* (dispense one scoop) and *Settings* (enable WiFi/AP for configuration).

## Environment Options
- VS Code Dev Container (`.devcontainer/`) provides an all-in-one PlatformIO toolchain; requires Docker Desktop/WSL2 on Windows.
- Docker CLI alternative:
  ```bash
  docker run --rm -it \
    -v "$PWD":/workspace -w /workspace \
    platformio/platformio-core pio run -e esp32c6
  ```
  Add `--device=/dev/ttyUSB0` (Linux/macOS) when flashing.
- Local PlatformIO Core/IDE remains supported; Makefile targets wrap common commands.
