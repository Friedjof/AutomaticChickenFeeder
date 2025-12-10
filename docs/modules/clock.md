# Clock Service

## Purpose
The ClockService wraps the DS3231 RTC module, providing accurate timekeeping, alarm management, and browser-based time synchronization.

## Hardware
- **DS3231 RTC**: High-precision I2C RTC with temperature compensation
- **I2C Bus**: SDA/SCL pins (board-specific)
- **Alarm Pin**: Interrupt output (GPIO 3)
- **Battery Backup**: CR2032 maintains time during power loss

## Responsibilities
- Initialize DS3231 via I2C
- Provide current time via `now()`
- Program hardware alarm for scheduled feeds
- Handle alarm interrupts and clear flags
- Sync time from browser timestamps
- Report RTC availability status

## Public API

```cpp
bool begin();                           // Initialize RTC
DateTime now();                         // Get current time
bool setTime(const DateTime &dt);      // Set RTC time
bool setAlarm(const DateTime &dt);     // Program alarm
bool clearAlarm();                      // Disable and clear alarm
bool checkAlarmFlag();                  // Check if alarm triggered
bool isAvailable() const;               // RTC connection status
```

## RTC Initialization

```cpp
bool ClockService::begin()
```

1. Initialize I2C bus
2. Detect DS3231 presence
3. Check if RTC lost power
4. If lost power: mark as requires sync
5. Disable alarm by default
6. Return availability status

## Time Management

### Getting Current Time
```cpp
DateTime now()
```

- Returns current time from DS3231
- DateTime format: year, month, day, hour, minute, second
- Unix timestamp available via `dt.unixtime()`
- Returns invalid DateTime if RTC unavailable

### Setting Time
```cpp
bool setTime(const DateTime &dt)
```

- Writes new time to DS3231
- Called by browser sync endpoint
- Validates DateTime before write
- Returns success/failure

## Alarm Management

### Programming Alarm
```cpp
bool setAlarm(const DateTime &dt)
```

1. Disables existing alarm
2. Calculates alarm registers from DateTime
3. Programs DS3231 Alarm 1
4. Enables alarm interrupt
5. Logs alarm time to serial
6. Returns success/failure

**DS3231 Alarm Capabilities:**
- Alarm 1: Seconds, minutes, hours, day/date
- Alarm 2: Minutes, hours, day/date (not used)
- Single alarm active at a time

### Clearing Alarm
```cpp
bool clearAlarm()
```

1. Clears alarm flag (`rtc.clearAlarm(1)`)
2. Disables alarm (`rtc.disableAlarm(1)`)
3. Logs to serial: "[CLOCK] Alarm cleared"
4. Called when no schedules active

### Checking Alarm Status
```cpp
bool checkAlarmFlag()
```

- Returns `rtc.alarmFired(1)`
- Called in main loop via SchedulingService
- True if alarm interrupt triggered

## Browser Time Sync

Triggered via WebService API: `POST /api/time`

### Sync Flow
1. Browser sends ISO timestamp
2. WebService parses to Unix timestamp
3. ClockService converts to DateTime
4. `setTime()` updates DS3231
5. Logs sync event to serial

### Format
- Input: ISO 8601 string ("2025-01-15T14:30:00Z")
- Converted to DateTime
- Written to DS3231 registers

## Deep Sleep Integration

### Before Sleep
- RTC alarm remains programmed in DS3231 hardware
- DS3231 maintains time on battery backup
- Alarm pin stays configured as interrupt source

### On Wake
- ESP32 wakes from GPIO interrupt (RTC_INT_PIN)
- SchedulingService calls `checkAlarmFlag()`
- ClockService confirms alarm triggered
- Alarm cleared via `clearAlarm()`

## Integration Points

### SchedulingService
- Calls `now()` for event generation
- Programs alarms via `setAlarm()`
- Checks alarm status via `checkAlarmFlag()`
- Clears alarms via `clearAlarm()`

### FeedingService
- Gets timestamps for feed history
- Calls `now()` via `recordFeedEvent()`

### WebService
- Receives browser time sync requests
- Formats DateTime to ISO 8601 strings
- Returns RTC status in API responses

### Main Application
- Calls `begin()` during setup
- Logs availability status

## Error Handling

### RTC Unavailable
- `begin()` returns false
- `available` flag set to false
- All operations return failure
- System continues without RTC (no scheduled feeds)
- Logged as "[WARN] DS3231 RTC not available"

### Lost Power
- Detected during `begin()`
- Logged as "[CLOCK] RTC lost power, time may be invalid"
- Requires browser sync to restore accurate time

### Invalid DateTime
- Validation before RTC writes
- Returns false on invalid input
- Logs error to serial

## Logging
All operations log to serial with `[CLOCK]` prefix:
- Initialization status
- Lost power detection
- Time sync events
- Alarm programming
- Alarm clearing
- Error conditions

## Testing

### Manual Testing
1. Power cycle device → check for lost power detection
2. Set alarm 2 minutes in future via schedule
3. Monitor serial for alarm programming
4. Wait for alarm trigger
5. Verify alarm flag set and cleared

### Browser Sync Testing
1. Open web UI
2. Monitor serial for sync logs
3. Verify time matches browser
4. Check RTC persists after power cycle

### Alarm Testing
1. Program alarm via SchedulingService
2. Enter deep sleep
3. Wake on RTC alarm
4. Verify alarm flag cleared
5. Check next alarm programmed
