# Source Tree Structure

This document describes the source tree structure for the **Automated Chicken 2.0** project.  
The goal is to provide a clear, modular, and maintainable project layout based on the ESP-IDF component model.

---

## 1. Top-Level Structure

```
AutomatedChickenFeeder/
│
├── CMakeLists.txt              # Main CMake entry point
├── sdkconfig                   # ESP-IDF project configuration
│
├── main/                       # Application entry point
│   ├── main.c                  # Initialization and top-level logic
│   ├── CMakeLists.txt
│
├── components/                 # Modular components
│   ├── feeding_service/        # Feeding mechanism logic
│   │   ├── include/
│   │   │   └── feeding_service.h
│   │   ├── src/
│   │   │   └── feeding_service.c
│   │   └── CMakeLists.txt
│   │
│   ├── sensor_service/         # VL53L0X sensor integration
│   │   ├── include/
│   │   │   └── sensor_service.h
│   │   ├── src/
│   │   │   └── sensor_service.c
│   │   └── CMakeLists.txt
│   │
│   ├── zigbee_service/         # ZigBee mock integration
│   │   ├── include/
│   │   │   └── zigbee_service.h
│   │   ├── src/
│   │   │   └── zigbee_service.c
│   │   └── CMakeLists.txt
│   │
│   ├── vibration_service/      # Vibration motor control (optional)
│   │   ├── include/
│   │   │   └── vibration_service.h
│   │   ├── src/
│   │   │   └── vibration_service.c
│   │   └── CMakeLists.txt
│   │
│   ├── rtc_service/            # Real-Time Clock handling (if external RTC is used)
│   │   ├── include/
│   │   │   └── rtc_service.h
│   │   ├── src/
│   │   │   └── rtc_service.c
│   │   └── CMakeLists.txt
│   │
│   └── ...
│
├── docs/                       # Documentation (Markdown)
│   ├── project_overview.md
│   ├── architecture/
│   │   ├── coding-standards.md
│   │   ├── tech-stack.md
│   │   └── source-tree.md
│   ├── software/
│   │   ├── feeding_service.md
│   │   ├── sensor_service.md
│   │   ├── zigbee_service.md
│   │   ├── vibration_service.md
│   │   └── rtc_service.md
│   └── development/
│       ├── user_stories.md
│       ├── roadmap.md
│       └── changelog.md
│
└── tools/                      # (Optional) Build or utility scripts

```

---

## 2. Component Guidelines
- **Purpose:** Each component handles one clear functionality.
- **Structure:**
```
/components/<component_name>/
├── include/                # Public headers
├── src/                    # Implementation
└── CMakeLists.txt
```
- **Naming Convention:** `snake_case` for directories and files.

---

## 3. Build and Flash
- Use ESP-IDF standard build and flash flow:
```bash
idf.py build flash monitor
```

---

## 4. Documentation Location

* All Markdown documentation is stored under `/docs`.
* Each component has its own documentation file:

  * `/docs/software/<component_name>.md`

---

## 5. Future Additions

* `/tests/` directory for unit and integration tests (optional).
* `/configs/` directory for default configuration files (if needed).
* `/firmware/` directory for pre-built firmware binaries.
