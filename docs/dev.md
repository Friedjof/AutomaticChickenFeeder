# Development Guide

This document summarises conventions and workflows for extending the PlatformIO-based firmware.

## Naming & File Layout
- Each service lives in its own pair of files inside `src/app/` and `include/app/`:
  - `src/app/<name>_module.cpp`
  - `include/app/<name>_module.hpp`
  Examples: `schedule_module.cpp`, `power_module.hpp`.
- Interfaces shared across modules (data models, events) go under `include/models/`; compile-time constants live in `include/config/`.
- Platform entry point lives in `src/main.cpp`; it should only construct `SystemController` and call `run()`.
- Keep filenames, class names, and namespaces aligned:
  ```cpp
  // include/app/schedule_module.hpp
  namespace app::schedule {
  class ScheduleModule : public IService { ... };
  }

  // src/app/schedule_module.cpp
  using namespace app::schedule;
  ```
- Header guards: use `#pragma once` for simplicity.

## Module Structure Template
Each module follows a common structure:
```cpp
// include/app/foo_module.hpp
#pragma once
#include "app/service_context.hpp"

namespace app::foo {
class FooModule : public IService {
public:
    bool init(ServiceContext& ctx) override;
    void loop(ServiceContext& ctx) override;
    void onWake(ServiceContext& ctx) override;
    void onSleep(ServiceContext& ctx) override;
private:
    // internal state
};
}

// src/app/foo_module.cpp
#include "app/foo_module.hpp"

using namespace app::foo;

bool FooModule::init(ServiceContext& ctx) {
    // hardware setup or state initialisation
    return true;
}

void FooModule::loop(ServiceContext& ctx) {
    // non-blocking state machine work
}

void FooModule::onWake(ServiceContext& ctx) {
    // optional: refresh hardware handles
}

void FooModule::onSleep(ServiceContext& ctx) {
    // optional: release resources
}
```
Guidelines:
- Keep module state private; expose getters/setters only when necessary.
- Avoid blocking calls; rely on timers, counters, or state machines.
- Use `ctx` to reach other services or shared utilities—never store raw references globally.

## Coding Conventions
- Language: C++17 (configure in `platformio.ini` if needed).
- Namespaces: group by domain (`app::controller`, `app::schedule`). Avoid `using namespace` in headers.
- Logging: centralise through a future `Logger` utility accessible via `ServiceContext`.
- Error handling: return `false` from `init()` or emit events through controller; do not `abort()` unless unrecoverable.
- Constants: prefer `constexpr` in config headers (e.g. `include/config/pins.hpp`).

## PlatformIO Setup
- Baseline `platformio.ini` (adjust as needed):
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
- Use `monitor_speed = 115200` for serial logs. Swap libraries only after updating docs. The `native` environment enables quick host builds/tests without installing ESP32 toolchains.

## Build & Test Workflow
1. Install dependencies (`platformio` CLI, optional `npm` for Web UI assets located in the future `web/` workspace) or open the repo in the Dev Container (`.devcontainer/`).
2. Run `make setup` to copy `data-template/` → `data/`, ensure `.pio-home/` exists, and prefetch packages.
3. Build firmware (host env by default):
   ```bash
   pio run             # native
   pio run -e esp32c6  # hardware build
   ```
4. Upload firmware to device:
   ```bash
   pio run -t upload -e esp32c6
   ```
5. Upload LittleFS assets:
   ```bash
   pio run -t uploadfs -e esp32c6
   ```
6. Run unit tests (hosted or on device):
   ```bash
   pio test            # native
   pio test -e esp32c6 # hardware
   ```

## Branching & Commits
- Use feature branches per module/feature (`feature/schedule-loop`, `fix/power-debounce`).
- Commits should be small and focused; include doc updates alongside code changes when relevant.
- When introducing new modules, create stub header/source, register in controller, and add tests where possible.

## Checklist for New Features
- [ ] Update or create module docs under `docs/modules/`.
- [ ] Extend interfaces in `include/` with forward declarations rather than includes where possible.
- [ ] Ensure `data-template/` reflects any new LittleFS assets or config keys.
- [ ] Add unit tests in `test/` for new logic (schedule calculations, state machines, etc.).
- [ ] Run `pio run` and `pio test` before committing.
- [ ] Update `docs/dev.md` if conventions or toolchains change.

## Tooling Tips
- Use `clang-format` (if configured) to keep code style consistent.
- `rg` (ripgrep) is preferred for searching within the repo.
- Keep VSCode settings under `.vscode/` minimal; rely on repo-level configuration.
- Dev Container users can add serial devices by editing `.devcontainer/devcontainer.json` (`runArgs` / `mounts`).

Following these guidelines keeps the codebase modular, testable, and easy to reason about as the PlatformIO transition progresses.
