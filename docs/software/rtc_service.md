# RTC Service - DS3231 Integration

## Overview
The RTC Service manages the DS3231 Real-Time Clock module via I2C communication, providing accurate timekeeping and alarm functionality for scheduled feeding operations.

## Hardware Requirements
- **DS3231 RTC Module** with I2C interface
- **Pin Connections**:
  - SDA: GPIO5 (D4)
  - SCL: GPIO6 (D5) 
  - VCC: 3.3V
  - GND: Ground
- **Battery Backup**: CR2032 for time retention during power loss

## Features
- High precision timekeeping (±2ppm accuracy)
- Temperature compensated crystal oscillator
- Battery backup support
- Alarm functions for scheduled events
- I2C interface at 3.3V logic level

## Public API

### Initialization
```c
esp_err_t rtc_service_init(void);
void rtc_service_deinit(void);
```

### Time Management
```c
// Set current time
esp_err_t rtc_service_set_time(struct tm* time_info);

// Get current time
esp_err_t rtc_service_get_time(struct tm* time_info);

// Check if RTC is running
bool rtc_service_is_running(void);
```

### Alarm Functions
```c
// Set alarm for specific time
esp_err_t rtc_service_set_alarm(uint8_t alarm_num, struct tm* alarm_time, bool repeat_daily);

// Enable/disable alarm
esp_err_t rtc_service_enable_alarm(uint8_t alarm_num, bool enable);

// Check alarm status
bool rtc_service_is_alarm_triggered(uint8_t alarm_num);

// Clear alarm flag
esp_err_t rtc_service_clear_alarm(uint8_t alarm_num);
```

### Temperature Reading
```c
// Get temperature from DS3231 (for compensation)
esp_err_t rtc_service_get_temperature(float* temperature);
```

## Implementation Details

### I2C Configuration
- **Address**: 0x68 (DS3231 standard address)
- **Speed**: 100kHz (standard I2C)
- **Pull-ups**: External 4.7kΩ resistors required

### Register Map (DS3231)
- 0x00-0x06: Time/Date registers
- 0x07-0x0A: Alarm 1 registers
- 0x0B-0x0D: Alarm 2 registers
- 0x0E: Control register
- 0x0F: Status register
- 0x11-0x12: Temperature registers

### Error Handling
- I2C communication failures
- Invalid time/date values
- Battery backup status monitoring

## Integration with Scheduler Service
The RTC Service works closely with the Scheduler Service:
1. RTC provides accurate time reference
2. Alarms trigger feeding events
3. Scheduler manages feeding schedules
4. Battery backup ensures schedule persistence

## Power Consumption
- Active: ~3.3μA @ 3.3V
- Backup: ~1.3μA @ 3.0V (battery mode)
- Extremely low power for continuous operation

## Known Limitations
- Requires external I2C pull-up resistors
- Temperature reading accuracy: ±3°C
- Alarm precision: 1 second minimum interval
- Two alarms maximum (DS3231 hardware limitation)