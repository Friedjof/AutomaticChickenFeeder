# Product Requirements Document (PRD)

## 1. Introduction

### 1.1 Product Overview
The Automated Chicken Feeder is an ESP32-C6-based device designed to automate the feeding process of poultry.  
It ensures reliable feeding schedules, monitoring of feed levels, remote configuration, and integration with smart home systems.

### 1.2 Objectives
- Automate daily chicken feeding routines to reduce manual workload.
- Provide reliable scheduling and feed level monitoring.
- Enable remote access and control via a web dashboard and/or smart home integration.

### 1.3 Stakeholders
- **End Users:** Small farm owners, backyard chicken keepers.
- **Maintenance Staff:** Responsible for refilling and cleaning.
- **System Administrators:** Users managing firmware updates and remote access.

---

## 2. User Stories / Use Cases

1. **Scheduled Feeding**
   - As a chicken keeper, I want to set daily feeding times so the system automatically dispenses food.
2. **Manual Feeding**
   - As a chicken keeper, I want to trigger feeding on demand via button or web interface.
3. **Low Feed Notification**
   - As a user, I want to be notified when feed levels are low so I can refill promptly.
4. **Remote Monitoring**
   - As a user, I want to check feed levels and feeder status remotely.
5. **Firmware Updates**
   - As a system administrator, I want to update the feeder software via OTA without physical access.

---

## 3. Functional Requirements

### 3.1 Feeding System
- Two servo motors to control a scoop:
  - **Servo 1**: Controls rotation.
  - **Servo 2**: Controls tilt.
- Feeding sequence:
  1. **Loading position:** Servo 1 = 0°, Servo 2 = 180°
  2. **Dispense feed:** Servo 1 = 180°, Servo 2 = 0° (3s)
  3. **Return to loading:** Servo 1 = 0°, Servo 2 = 180° (3s)

### 3.2 Scheduling
- User-defined daily feeding times.
- Option to override schedule manually.

### 3.3 Sensors & Monitoring
- Load cell for feed level measurement.
- Optional environmental sensor (temperature, humidity).

### 3.4 User Interface
- Web dashboard hosted on the ESP32-C6.
- Live display of feed levels and system status.
- Manual feeding trigger and configuration management.

### 3.5 Connectivity
- Wi-Fi connection for dashboard and notifications.
- Optional MQTT for Home Assistant integration.

### 3.6 Notifications
- Low feed alerts via email or Telegram.

### 3.7 Firmware Updates
- OTA (Over-the-Air) update capability.

---

## 4. Non-Functional Requirements

- **Reliability:** System must perform scheduled feedings consistently without user intervention.
- **Safety:** Emergency stop function in case of servo jam.
- **Power:** Support for continuous operation from AC adapter or solar-powered setup.
- **Scalability:** Code structure must allow adding new sensors or modules.
- **Security:** Wi-Fi dashboard must be password-protected.

---

## 5. Constraints and Assumptions

- Servos are 5V MG90S class.
- ESP32-C6 is the standard microcontroller.
- Internet access is optional (local operation must still work).
- System enclosure is weather-protected.

---

## 6. Acceptance Criteria

- The system successfully dispenses feed at scheduled times within ±10s accuracy.
- Manual feeding works from both the physical button and the web dashboard.
- Low feed notification triggers when feed level drops below a configurable threshold.
- OTA update process completes without physical intervention.
- Dashboard is accessible via Wi-Fi from a smartphone or PC.

---

## 7. Out of Scope

- Egg collection, cleaning automation.
- Integration with third-party cloud platforms (except MQTT/Home Assistant).

---

## 8. References

- [docs/architecture.md](./architecture.md)
- ESP32-C6 technical reference
- MG90S datasheet
