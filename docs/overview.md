# Automated Chicken 2.0 – Project Documentation

## 1. Project Overview
Automated Chicken 2.0 is a next-generation automated chicken feeder designed for precise portioning, energy efficiency, and modular extendibility.  
The project is based on the ESP32C6 using the Espressif IoT Development Framework (ESP-IDF).

**Key features:**
- Precise feed portioning using a dual-servo scoop mechanism.
- Manual feeding via button press.
- Configurable feeding schedules with persistent storage.
- Mobile-first web interface (Access Point mode).
- Optional ZigBee integration for Smart Home systems.
- Future support for vibration motor and feed flow sensors.

---

## 2. Functional Requirements

### 2.1 Core Features (MVP)
1. **Manual Feeding Button**
   - Triggers immediate feeding.
   - Sets status back to `READY` after completion.

2. **Feeding Mechanism**
   - Dual MG90S servo motors controlling the scoop.
   - Kipping angles:
     - Servo A: 0° → 180° (dump) → 0° (reset)
     - Servo B: 180° → 0° (dump) → 180° (reset)
   - 3 additional small shake movements (±10°) during dumping.

3. **Power Saving**
   - Servo power supply switched off when idle via NPN transistor on VCC.
   - (TODO: finalize transistor selection – see Hardware section)

4. **Status Management**
   - Simple status states: `READY`, `FEEDING`, `ERROR`.

---

### 2.2 Extended Features (Roadmap)
- **Vibration Motor** to prevent feed clogging (optional, low priority).
- **Distance Sensor (VL53L0X)** to confirm feed output.
- **Configuration Interface (Web UI)**
  - Time and routine scheduling.
  - Feed amount per routine.
  - JSON-based import/export.
- **Persistent Storage** (LittleFS or NVS).
- **ZigBee Integration (Mock first)**
  - Prepare component for later DeCONZ/ZigBee2MQTT/Home Assistant integration.

---

### 2.3 Non-Functional Requirements
- Modular software architecture using ESP-IDF component model.
- Documentation in Markdown under `/docs`.
- Code and documentation in English.
- Energy efficient: Deep Sleep + servo power off.
- Ready-to-use binary (no config flashing required).

---

## 3. Priorities (Development Phases)

### Phase 1 – MVP
- Button-triggered feeding routine.
- Servo transistor power switch.
- Shake effect during dump.
- Documentation skeleton (`/docs`).

### Phase 2 – Time Management & Reliability
- **DS3231 Real-Time Clock integration** for accurate timekeeping.
- **Scheduled feeding routines** based on RTC time.
- Vibration motor control (optional).
- Scoop motion verification (sensor-based).

### Phase 3 – Web Interface & Persistence
- Mobile-first UI (Access Point mode).
- JSON config storage.
- Import/Export.

### Phase 4 – IoT & Analytics
- ZigBee mock integration.
- Basic event feedback to Smart Home (Home Assistant focus).
- Optional log interface (low write, debug).

---

## 4. Hardware

### 4.1 Core Components
- **MCU:** ESP32C6 (Wi-Fi + ZigBee).
- **Servos:** 2× MG90S.
- **RTC:** DS3231 I2C Real-Time Clock (GPIO5/SDA, GPIO6/SCL).
  - High precision (±2ppm) with temperature compensation.
  - Battery backup (CR2032) for time retention during power loss.
  - Alarm functions for scheduled feeding wake-up.
- **Distance Sensor:** VL53L0X ToF sensor.
- **Vibration Motor:** Optional, activated before feeding.
- **Buttons:** Minimum 2 (Feed / AP Mode).

### 4.2 Power Switching
- **Transistor:** NPN-based high-side switching of servo VCC.
- Recommended NPN types (common, easily available):
  - **2N2222A**
  - **BC337**
  - **S8050**
- TODO: Final selection based on availability and current handling.

### 4.3 Power Supply
- Default: USB-C power supply (also used for flashing).
- Future: optional battery via built-in charging circuit (model aircraft LiPo supported).

---

## 5. Software Architecture

### 5.1 Development Framework
- **ESP-IDF** (C/C++)
- **CMake** build system
- **FreeRTOS** (built-in with ESP-IDF)

### 5.2 Component Model (ESP-IDF)
- Each major function is implemented as a separate component:
  - `feeding_service` → scoop movement logic
  - `rtc_service` → DS3231 I2C communication and time management
  - `scheduler_service` → feeding schedule management and alarm handling
  - `sensor_service` → VL53L0X distance measurement
  - `vibration_service` → vibration motor control (optional)
  - `zigbee_service` → ZigBee mock integration

### 5.3 Naming Conventions
- Functions: `<component>_<function>()`
  - Example: `feeding_service_start_feeding()`
- File names: snake_case
  - Example: `feeding_service.c`
- Documentation:
  - `/docs/software/<component>.md` → per component documentation

---

## 6. Web Interface (Phase 3)
- Runs on ESP32C6 in Access Point mode.
- **Core features:**
  - Configure feeding times & routines (weekday-based, named routines).
  - Adjust feed amount.
  - Import/Export configuration (JSON).
- **Persistent storage:** JSON saved on flash (LittleFS or NVS).
- **UI Requirements:** mobile-first design, minimal dependencies.

---

## 7. Documentation Structure
```

/docs
├── project_overview.md   ← this file
├── hardware/
│   ├── overview.md
│   ├── sensors.md
│   └── power.md
├── software/
│   ├── feeding_service.md
│   ├── rtc_service.md
│   ├── scheduler_service.md
│   ├── sensor_service.md
│   ├── zigbee_service.md
│   └── vibration_service.md
├── development/
│   ├── conventions.md
│   ├── changelog.md
│   └── roadmap.md

```

### 7.1 Conventions
- Markdown (UTF-8, GitHub-compatible).
- Header hierarchy: `#`, `##`, `###`.
- Diagrams/images under `/docs/images/`.
- Every component has:
  - Purpose
  - Public API description
  - Internal flow (optional)
  - Known limitations

