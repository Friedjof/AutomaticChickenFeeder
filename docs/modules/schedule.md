# Scheduling Service

## Purpose
The SchedulingService manages the feeding schedule by generating timer events from configured schedules, programming RTC alarms, and triggering feeds at the appropriate times.

## Responsibilities
- Generate timer events from enabled schedules
- Program DS3231 RTC alarm for next due event
- Handle RTC alarm interrupts and execute feeds
- Regenerate events when configuration changes
- Maintain event queue for upcoming feeds

## Hardware
- **DS3231 RTC**: Hardware alarm capability
- **RTC Interrupt Pin** (GPIO 3): Wakes ESP32 from deep sleep
- Event generation covers next 7 days

## Data Structures

### Timer Event
```cpp
struct TimerEvent {
    DateTime timestamp;    // When feed should occur
    uint8_t scheduleId;   // Which schedule triggered this (1-6)
    uint8_t portionUnits; // How many portions to dispense
    bool valid;           // Event is active
};
```

### Schedule Entry
```cpp
struct Schedule {
    uint8_t id;              // 1-6
    bool enabled;            // Active/inactive
    char time[6];            // "HH:MM" format
    uint8_t weekday_mask;    // Bit 0=Sunday, 6=Saturday
    uint8_t portion_units;   // 1-5 units per feed
};
```

## Public API

```cpp
void begin();              // Initialize, generate events, program first alarm
void update();             // Check for RTC alarm flag (call in loop)
void onConfigChanged();    // Regenerate events after schedule changes
void checkAlarm();         // Handle alarm trigger, execute due feeds
```

## Event Generation

### Algorithm
1. Load all 6 schedules from ConfigService
2. For each enabled schedule:
   - Calculate next occurrence starting from now
   - Generate occurrences for next 7 days
   - Add to timer event queue (max 50 events)
3. Sort events by timestamp
4. Program RTC alarm for earliest future event

### Next Occurrence Calculation
```cpp
DateTime getNextOccurrence(const Schedule &schedule, const DateTime &from)
```

- Parses time from "HH:MM" format
- Checks if today's occurrence is still future
- Iterates through next 7 days to find matching weekday
- 60-second grace window for events just triggered
- Returns invalid DateTime if no match found

### Weekday Matching
```cpp
bool isActiveOnWeekday(uint8_t weekdayMask, uint8_t weekday)
```

- weekday: 0=Sunday, 1=Monday, ..., 6=Saturday
- weekdayMask: bit field (bit 0=Sunday, bit 6=Saturday)
- Returns true if corresponding bit is set

**Example:** weekdayMask = 62 (0b0111110) = Monday–Friday

## RTC Alarm Management

### Programming Next Alarm
```cpp
void programNextAlarm()
```

1. Find next future event from queue
2. If no events exist:
   - Clear RTC alarm
   - Log "alarm disabled"
   - Return
3. Program RTC with event timestamp via ClockService
4. Log scheduled event details

### Alarm Handling
```cpp
void checkAlarm()
```

Called when RTC alarm triggers:
1. Clear RTC alarm flag via ClockService
2. Find all due events (timestamp ≤ now)
3. For each due event:
   - Call `handleTimerEvent()`
   - Mark event as invalid
4. Program next alarm

### Feed Execution
```cpp
void handleTimerEvent(TimerEvent &event)
```

- Triggers `feedingService.feed(event.portionUnits)`
- Logs schedule ID and portion count
- FeedingService handles actual dispensing

## Configuration Change Flow

When schedules are updated via API:
1. WebService receives POST to `/api/config`
2. ConfigService saves new schedules to NVS
3. WebService calls `schedulingService.onConfigChanged()`
4. SchedulingService:
   - Regenerates all timer events
   - Programs next alarm
   - Old events are discarded

## Integration Points

### ConfigService
- Loads schedules via `loadAllSchedules()`
- Called once at `begin()` and on config changes

### ClockService
- Gets current time via `now()`
- Programs alarm via `setAlarm(DateTime)`
- Clears alarm via `clearAlarm()`
- Checks alarm flag via `checkAlarmFlag()` and `alarmFired()`

### FeedingService
- Triggers feeds via `feed(portionUnits)`
- Monitors `isFeeding()` to track completion

### Main Application
- Calls `update()` in main loop
- Calls `checkAlarm()` on RTC wake
- Prevents sleep while feeding active

## Deep Sleep Integration

### Before Sleep
- Current timer events remain in RAM (lost during deep sleep)
- RTC alarm stays programmed in DS3231 hardware
- DS3231 maintains time and triggers interrupt

### On Wake (RTC Alarm)
1. ESP32 wakes from GPIO interrupt (RTC_INT_PIN)
2. Main calls `schedulingService.checkAlarm()`
3. Due events are executed
4. Events regenerated from schedules
5. Next alarm programmed
6. System returns to sleep

## Event Queue Management

### Constants
- `MAX_TIMER_EVENTS`: 50 events maximum
- Coverage: 7 days forward
- Auto-regeneration on config change

### Limits
- 6 schedules × 7 days = up to 42 events per schedule
- Typical usage: 3-5 active schedules = 21-35 events

## Error Handling

### No Valid Events
- `programNextAlarm()` disables RTC alarm
- System stays in deep sleep until button wake
- Logged as "[SCHED] No future events - alarm disabled"

### Schedule Load Failure
- Logs error to serial
- Returns without programming alarm
- System remains functional with previous events

### Invalid Time/Weekday
- `getNextOccurrence()` returns invalid DateTime
- Event not added to queue
- Continues with other schedules

## Testing

### Manual Testing
1. Enable schedule via web UI
2. Set time 2 minutes in future
3. Monitor serial: "[SCHED] Added event: Schedule X at YYYY-MM-DD HH:MM"
4. Wait for alarm trigger
5. Verify feed execution

### Schedule Validation
- Test weekday masks: single day, range, all days
- Test time wrapping: late night → next morning
- Test disabled schedules: should not generate events
- Test multiple schedules at same time

### Alarm Programming
- Verify alarm clears when all schedules disabled
- Verify alarm updates on config change
- Test deep sleep wake via RTC alarm

## Logging
All operations log to serial with `[SCHED]` prefix:
- Event generation count
- Individual event timestamps
- Alarm programming confirmations
- Alarm trigger notifications
- Feed execution details
