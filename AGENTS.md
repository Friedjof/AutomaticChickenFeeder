# Repository Guidelines

This repository contains an **ESP32-C3 automatic chicken feeder** project using PlatformIO. The goal is to provide a functional feeding system with button control and servo actuation.

## Build & Flash

**IMPORTANT:** Always use `make build` (or just `make`) instead of `pio run` directly!

### Why Use the Makefile?

The Makefile does **more than just compile firmware**. It automates the complete build pipeline:

1. **Web UI Build** (`web-headers` target):
   - Runs `npm install` in `/web` directory
   - Builds Vite project → generates optimized `dist/` files
   - Converts HTML/CSS/JS to gzipped C header files
   - Embeds web interface directly into firmware flash memory

2. **Firmware Compilation**:
   - Compiles ESP32-C3 firmware with embedded web files
   - Links all libraries (Button2, RTClib, ESPAsyncWebServer, etc.)

**The dependency chain:**
```
make build → web-headers → npm build → web-to-header.py → pio run
```

**What happens if you skip `make` and use `pio run` directly?**
- ❌ Web UI won't be rebuilt
- ❌ Old/outdated web files stay embedded in firmware
- ❌ Frontend changes (HTML/CSS/JS) won't appear on device
- ❌ You'll waste time debugging why your web changes don't work

### Basic Commands
- `make` / `make build` – **FULL build:** web UI + firmware (ALWAYS use this!)
- `make clean` – clean build artifacts
- `make web-headers` – rebuild only web UI headers (useful for frontend-only changes)

### Device Port Management
The Makefile supports automatic port detection and manual port selection:

- `make flash` – flash with auto-detected port
- `make flash 1` – flash to /dev/ttyACM1 
- `make monitor` – serial monitor with auto-detected port
- `make monitor 2` – monitor /dev/ttyACM2
- `make run` – flash + monitor (auto-detect)
- `make run 1` – flash + monitor on /dev/ttyACM1
- `make list` – list available ESP32 devices with port numbers

### Port Selection Examples
```bash
# Auto-detect port (recommended for single device)
make flash
make monitor

# Specific port (useful with multiple ESP32 devices)
make flash 2      # Flash to /dev/ttyACM2
make monitor 2    # Monitor /dev/ttyACM2
make run 1        # Flash and monitor /dev/ttyACM1

# List connected ESP32 devices
make list
```

## Hardware Setup
- **Board**: Seeed Xiao ESP32-C3
- **Servos**: Connected to GPIO 2 and GPIO 3
- **Button**: Pin defined in ButtonService (check ButtonService.hpp)
- **Power**: Ensure adequate power supply for servo operation

## Code Structure
- `src/main.cpp`: Main application with button handling
- `lib/ButtonService/`: Button input management using Button2 library
- `lib/FeedingService/`: Dual servo control for feeding mechanism
- `lib/WebService/`: Placeholder for future web interface

## Platform Configuration
- `platformio.ini` uses standard `espressif32` platform for ESP32-C3
- Board target: `seeed_xiao_esp32c3`
- Upload speed: 460800 baud for fast flashing
- Dependencies: ESP32Servo v3.0.9+ and Button2 v2.5.0+

## Development Guidelines

### Build Workflow
**After ANY code change (backend OR frontend), ALWAYS run:**
```bash
make build    # or just: make
```

**Common mistake:** Running `pio run` directly skips web build!

### Frontend Development
If you only changed web files (`web/src/*.js`, `web/index.html`, etc.):
```bash
make web-headers    # Faster: only rebuild web UI
pio run             # Then compile firmware
```

### Backend Development
If you only changed firmware code (`src/`, `lib/*/`):
```bash
make build    # Still use make! It's safe and ensures web files are up-to-date
```

### Other Guidelines
- Test servo operation with appropriate power supply
- Ensure `.pio-home/penv` exists (create via `python3 -m venv .pio-home/penv` if needed)
- Keep code modular with separate service classes
- Never commit `web/dist/` or `web/node_modules/` (in .gitignore)
- Commit `lib/WebService/generated/web_files.h` for releases only

## Current Functionality
The system provides:
1. **Button Control:**
   - **Single click:** Start AP mode for configuration
   - **Double click:** Manual feed (1 portion)
   - **Long press (~600ms):** Enter deep sleep immediately
   - **Wakeup from sleep:** Button press wakes device + starts AP mode after 1 second delay
   - **Hold during boot (optional):** Enter maintenance mode (keeps WiFi always on, useful for debugging)

2. **Web Interface:**
   - Schedule configuration (6 feeding times with weekday masks)
   - Manual feed button
   - Time synchronization (browser → RTC, every 10s while page open)
   - Deep sleep control via button
   - Config import/export (JSON)
   - **OTA firmware update** (upload .bin files directly via browser)
   - Embedded in firmware (no SD card needed)

3. **RTC-Based Scheduling:**
   - DS3231 external RTC with alarm interrupts (GPIO3)
   - Automatic feeding at scheduled times
   - Timezone support (Europe/Berlin, UTC+1)
   - Event queue for next 7 days
   - Weekday filtering (e.g., Mon-Fri only)

4. **Power Management:**
   - **Deep sleep mode** to save battery (~10-20µA)
   - **Dual wakeup sources:**
     - Button (GPIO4 LOW) → Wakes + starts AP mode
     - RTC Alarm (GPIO3 LOW) → Wakes + feeds + returns to sleep
   - **Automatic sleep:**
     - After 2 minutes inactivity (no button, no web client)
     - Immediately after RTC-triggered feeding
   - **Manual sleep:** Long-press button
   - **Stay-awake conditions:**
     - While AP active with connected client
     - While feeding in progress
     - Any button press resets 2-minute inactivity timer

5. **Feeding Mechanism:**
   - Dual servo control (GPIO2, GPIO21)
   - Configurable portion sizes (1-5 units, 12g per unit)
   - Synchronized open/close sequence
   - Transistor control for servo power (GPIO5)
   - Multiple portions execute back-to-back

6. **OTA Firmware Updates:**
   - Available anytime when WiFi AP is active
   - Web-based firmware upload via browser
   - Progress tracking with visual feedback
   - Automatic device reboot after successful update
   - Partition scheme: `min_spiffs.csv` (2x ~1.9MB app partitions)
   - Configuration preserved during update (stored in NVS)

## Architecture Overview

### Embedded Web Interface
The web UI is **compiled into the firmware** using this pipeline:
```
web/src/*.js → Vite build → dist/*.js,css,html →
  Python script (gzip) → lib/WebService/generated/web_files.h →
    ESP32 firmware (.bin)
```

This means:
- ✅ No external filesystem needed
- ✅ Web interface stored in flash memory
- ✅ Served via AsyncWebServer at runtime
- ✅ 75% compression (31KB → 7.7KB)

### Service Architecture
```
main.cpp
├── ButtonService (GPIO4)
│   ├── Single click → Manual feed
│   ├── Double click → Start AP mode
│   └── Long press → Enter deep sleep
├── FeedingService (GPIO2, GPIO21, GPIO5)
│   ├── Dual servo control
│   ├── Transistor power switching
│   └── Multi-portion feeding
├── ClockService (I2C @ 0x68)
│   ├── DS3231 RTC time management
│   ├── Timezone conversion (UTC → CET/CEST)
│   └── Alarm programming
├── ConfigService (NVS)
│   ├── 6 schedules storage
│   └── Portion unit configuration
├── SchedulingService
│   ├── Event queue (50 events, 7 days)
│   ├── Weekday mask filtering
│   └── RTC alarm management
└── WebService (WiFi AP)
    ├── AsyncWebServer (port 80)
    ├── Captive portal DNS
    ├── REST API endpoints
    └── Time sync (browser → RTC)
```

### Deep Sleep & Wakeup Flow

```
┌─────────────────────────────────────────────────────────────┐
│                        DEEP SLEEP                           │
│                  (Power consumption: ~10-20µA)              │
└─────────────────────────────────────────────────────────────┘
                              │
                 ┌────────────┴────────────┐
                 │                         │
          [Button Press]            [RTC Alarm INT]
           (GPIO4 LOW)               (GPIO3 LOW)
                 │                         │
                 ▼                         ▼
        ┌─────────────────┐      ┌─────────────────┐
        │  Button Wakeup  │      │  RTC Wakeup     │
        └─────────────────┘      └─────────────────┘
                 │                         │
                 ▼                         ▼
        • Wait 1s delay          • Execute scheduled feed
        • Start AP mode          • Clear RTC alarm flag
        • 2s ignore window       • Process all due events
        • Mark activity
                 │                         │
                 ▼                         ▼
        ┌─────────────────┐      ┌─────────────────┐
        │   Active Mode   │      │ Return to Sleep │
        └─────────────────┘      └─────────────────┘
                 │                         │
        ┌────────┴─────────┐              │
        │ User Actions:    │              │
        │ • Single click   │              │
        │   → Start AP     │              │
        │ • Double click   │              │
        │   → Feed         │              │
        │ • Long press     │              │
        │   → Sleep        │              │
        │ • Web activity   │              │
        └──────────────────┘              │
                 │                         │
        ┌────────┴─────────┐              │
        │ Stay Awake If:   │              │
        │ • AP has clients │              │
        │ • Feeding active │              │
        │ • Activity <2min │              │
        └──────────────────┘              │
                 │                         │
        ┌────────┴─────────┐              │
        │ Sleep Triggers:  │              │
        │ • Long press     │              │
        │ • 2min timeout   │              │
        └──────────────────┘              │
                 │                         │
                 └────────────┬────────────┘
                              ▼
                        DEEP SLEEP
```

### GPIO Pin Assignments

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| GPIO2 | Servo 1 | Output | PWM control |
| GPIO3 | RTC INT | Input (PULLUP) | Alarm wakeup |
| GPIO4 | Button | Input (PULLUP) | User input + wakeup |
| GPIO5 | Transistor | Output | Servo power control |
| GPIO21 | Servo 2 | Output | PWM control |
| I2C SDA | DS3231 | I/O | RTC communication |
| I2C SCL | DS3231 | I/O | RTC communication |

### Power Consumption Estimates

| State | Current Draw | Duration | Notes |
|-------|--------------|----------|-------|
| **Deep Sleep** | ~10-20µA | Hours/Days | Main power saving mode |
| **Active (idle)** | ~80mA | 0-2 minutes | WiFi AP active, no servos |
| **Feeding** | ~500mA peak | ~3s per portion | Servo movement + holding torque |
| **Web Config** | ~120mA | User dependent | AP + web server + browser requests |

**Battery Life Example (2000mAh battery):**
- Deep sleep 23.5h/day: ~2000mAh / 0.02mA = ~100,000 hours ≈ **11 years**
- Active 30min/day (feeding): ~2000mAh / (0.02mA×23.5h + 80mA×0.5h) = **~50 days**

Follow these conventions to maintain code quality and enable easy hardware replication.
