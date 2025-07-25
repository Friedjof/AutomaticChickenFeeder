
# Automated Chicken Feeder

An ESP32-C6-based automated chicken feeding system with dual-servo actuation and advanced scheduling, monitoring, and control features.

> This project contains AI-generated code and documentation.

---

## Hardware

- **Microcontroller**: Seeed XIAO ESP32-C6
- **Servos**: 2x Standard 180° Servos (e.g., MG90S)
  - Servo 1 (Scoop rotation): GPIO18 (D10/MOSI)
  - Servo 2 (Scoop tilt): GPIO19 (D8/SCK)
- **Manual Control**: Button on GPIO1 (D1)
- **Power Control**: NPN Transistor (2N2222) on GPIO21 (D3)
- **Sensors**:
  - Load cell (HX711) for feed weight monitoring
  - Optional temperature/humidity sensor (DHT22)
- **Power Supply**:
  - AC adapter or solar power depending on installation

---

## System Operation

The feeding system uses two servos to control a scoop:

1. **Loading Position**: Servo 1 = 0°, Servo 2 = 180°
2. **Emptying Position**: Servo 1 = 180°, Servo 2 = 0° (3 seconds)
3. **Return to Loading**: Servo 1 = 0°, Servo 2 = 180° (3 seconds)

### Additional Features
- **Scheduled Feeding** via configurable timetable
- **Manual Feeding** via physical button or web interface
- **Feed Level Monitoring** with notifications on low stock
- **OTA Updates** for firmware upgrades without physical access
- **Web Dashboard** for configuration, monitoring, and manual control
- **MQTT/Home Assistant Integration** for smart home connectivity

---

## Project Structure

```

├── components/
│   └── feeding/                # Feeding control component
│       ├── CMakeLists.txt
│       ├── idf\_component.yml   # Servo dependency
│       ├── include/
│       │   └── feeding\_component.h
│       └── feeding\_component.c
├── main/
│   ├── AutomatedChickenFeeder.c
│   └── CMakeLists.txt
└── managed\_components/
└── espressif\_\_servo/       # ESP-IDF Servo library

````

---

## API – Feeding Component

```c
#include "feeding_component.h"

// Initialize feeding system
esp_err_t feeding_init(void);

// Trigger feeding cycle
esp_err_t feeding_start(void);

// Run feeding process in main loop (non-blocking)
void feeding_process(void);

// Check readiness
bool feeding_is_ready(void);
feeding_state_t feeding_get_state(void);
````

### Feeding States

* `FEEDING_STATE_IDLE` – Ready for next feeding
* `FEEDING_STATE_INIT` – Initialization phase
* `FEEDING_STATE_EMPTYING` – Dispensing feed (3s)
* `FEEDING_STATE_LOADING` – Returning scoop to loading position (3s)
* `FEEDING_STATE_READY` – Feeding cycle completed

---

## Build & Flash

```bash
# Build project
idf.py build

# Flash to ESP32-C6
idf.py flash

# Open serial monitor
idf.py monitor
```

---

## Development Environment

**VS Code Setup:**

* ESP-IDF Extension installed
* clangd for IntelliSense (recommended)
* C/C++ IntelliSense disabled to avoid conflicts

**Dependencies:**

* ESP-IDF v5.4+
* espressif/servo ^0.1.0

---

## Features Overview

* **Non-blocking state machine** – no delays in main loop
* **Modular component structure** – easily reusable
* **Scheduling and notifications** – configurable feeding times & alerts
* **OTA & Web Dashboard** – remote updates and management
* **Professional ESP-IDF design** – follows best practices

---

## Documentation

Detailed documentation covering hardware setup, software design, API references, and deployment guides is available in the [docs/](./docs) directory.
