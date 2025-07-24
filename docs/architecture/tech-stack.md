# Technology Stack

This document defines the technology stack used in the **Automated Chicken 2.0** project.  
It outlines the development environment, frameworks, and libraries used to build and maintain the system.

---

## 1. Core Platform
- **Microcontroller:** ESP32C6  
  - Integrated Wi-Fi (2.4 GHz) and ZigBee (IEEE 802.15.4) support
  - Built-in support for low-power operation (deep sleep)
- **Power Supply:** USB-C (primary) with optional LiPo battery support

---

## 2. Development Environment
- **Framework:** [Espressif IoT Development Framework (ESP-IDF)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)  
- **Language:** C / C++  
- **Build System:** CMake (native in ESP-IDF)  
- **RTOS:** FreeRTOS (included in ESP-IDF)  
- **Flashing & Monitoring Tool:**  
  - Use the ESP-IDF integrated workflow:  
    ```bash
    idf.py build flash monitor
    ```

---

## 3. Hardware Components
- **Servos:** 2 × MG90S (0°–180° range)
- **Distance Sensor:** VL53L0X (Time-of-Flight)
- **Vibration Motor (optional):** For feed vibration before each feed cycle
- **NPN Transistor:** Controls power to servo motors to reduce idle consumption  
  - Candidate types: 2N2222A, BC337, S8050 (final selection TBD)
- **Buttons:** Minimum 2 (Feed trigger / AP mode)

---

## 4. Communication Protocols
- **Wi-Fi (AP Mode):**  
  Used for initial configuration via a built-in web server.
- **HTTP (REST-like):**  
  Web interface communication (import/export JSON, schedule configuration).
- **ZigBee (future):**  
  Planned for Smart Home integration (Home Assistant via ZigBee2MQTT or DeCONZ).

---

## 5. Persistent Storage
- **JSON Configuration Files:**  
  - Stores feeding schedules, feed amounts, and named routines.
- **Storage Options:**  
  - LittleFS (preferred for file-like access)
  - NVS (for simple key/value pairs)

---

## 6. Web Interface (Phase 3)
- **Frontend:**  
  - Mobile-first design, optimized for smartphones.
  - Minimal dependencies (Vanilla JS, CSS).
- **Backend:**  
  - Runs on ESP32C6 integrated web server (via ESP-IDF HTTP server library).
- **Features:**  
  - Feeding schedule configuration
  - Feed amount configuration
  - Import/Export JSON

---

## 7. Development Tools
- **Version Control:** Git (GitHub repository planned)  
- **IDE (optional):** Visual Studio Code or CLion with ESP-IDF plugin  
- **Formatting:** clang-format or ESP-IDF astyle script  
- **Static Analysis:** cppcheck (optional)

---

## 8. Test & Debug
- **Serial Debugging & Monitoring:** via `idf.py monitor` (USB-C UART)
- **Optional:** Logic analyzer or oscilloscope for servo and sensor signals
- **Planned:** Unit tests for critical modules

---

## 9. Future Extensions
- ZigBee cluster definition for Home Assistant integration
- OTA (Over-the-Air) firmware updates
- Extended sensor support (feed level detection)
