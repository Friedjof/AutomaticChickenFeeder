# Scheduler Service - Feeding Schedule Management

## Overview
The Scheduler Service manages feeding schedules and coordinates with the RTC Service to trigger automatic feeding events at predetermined times.

## Features
- Multiple daily feeding schedules
- Weekday-specific feeding patterns
- Persistent schedule storage (NVS)
- RTC alarm integration
- Manual override capability
- Schedule validation and error handling

## Public API

### Initialization
```c
esp_err_t scheduler_service_init(void);
void scheduler_service_deinit(void);
```

### Schedule Management
```c
// Add feeding schedule
esp_err_t scheduler_service_add_schedule(schedule_entry_t* schedule);

// Remove feeding schedule by ID
esp_err_t scheduler_service_remove_schedule(uint8_t schedule_id);

// Get all active schedules
esp_err_t scheduler_service_get_schedules(schedule_entry_t* schedules, uint8_t* count);

// Clear all schedules
esp_err_t scheduler_service_clear_schedules(void);
```

### Schedule Control
```c
// Enable/disable scheduler
esp_err_t scheduler_service_enable(bool enable);

// Check if scheduler is active
bool scheduler_service_is_enabled(void);

// Force manual feeding (bypass schedule)
esp_err_t scheduler_service_manual_feed(void);
```

### Schedule Processing
```c
// Process pending alarms (called from main loop)
void scheduler_service_process(void);

// Get next scheduled feeding time
esp_err_t scheduler_service_get_next_feeding(struct tm* next_time);
```

## Data Structures

### Schedule Entry
```c
typedef struct {
    uint8_t id;                    // Unique schedule ID
    uint8_t hour;                  // Hour (0-23)
    uint8_t minute;                // Minute (0-59)
    uint8_t weekdays;              // Bitmask: bit 0=Sunday, bit 6=Saturday
    bool enabled;                  // Schedule active flag
    char name[32];                 // Human-readable name
} schedule_entry_t;
```

### Weekday Bitmask
```c
#define SCHEDULER_SUNDAY    (1 << 0)
#define SCHEDULER_MONDAY    (1 << 1)
#define SCHEDULER_TUESDAY   (1 << 2)
#define SCHEDULER_WEDNESDAY (1 << 3)
#define SCHEDULER_THURSDAY  (1 << 4)
#define SCHEDULER_FRIDAY    (1 << 5)
#define SCHEDULER_SATURDAY  (1 << 6)
#define SCHEDULER_WEEKDAYS  (0x3E)  // Mon-Fri
#define SCHEDULER_WEEKEND   (0x41)  // Sat-Sun
#define SCHEDULER_DAILY     (0x7F)  // All days
```

## Implementation Details

### Schedule Storage
- **Persistent Storage**: NVS (Non-Volatile Storage)
- **Key**: "feeding_schedules"
- **Format**: Binary array of schedule_entry_t structures
- **Maximum**: 16 schedules (configurable)

### RTC Integration
1. Calculate next feeding time from active schedules
2. Set DS3231 alarm for earliest upcoming feeding
3. Monitor alarm triggers in main loop
4. Recalculate and set next alarm after feeding

### Schedule Validation
- Time range validation (0-23h, 0-59m)
- Weekday bitmask validation
- Duplicate schedule detection
- Maximum schedule limit enforcement

### Feeding Execution Flow
1. RTC alarm triggers
2. Scheduler validates current time vs. schedule
3. Calls feeding_service to execute feeding
4. Logs feeding event
5. Calculates and sets next alarm

## Integration Points

### RTC Service
- Time reference for schedule calculations
- Alarm configuration for feeding triggers
- Battery backup ensures schedule persistence

### Feeding Service
- Execute actual feeding mechanism
- Feedback on feeding completion/failure
- Manual feeding override capability

### Web Interface (Phase 3)
- Schedule configuration and management
- Real-time schedule display
- Manual feeding controls
- Schedule import/export (JSON)

## Configuration Options
```c
#define SCHEDULER_MAX_SCHEDULES     16
#define SCHEDULER_NAME_MAX_LENGTH   32
#define SCHEDULER_ALARM_ADVANCE_SEC 5   // Set alarm 5s early for processing time
```

## Error Handling
- Invalid schedule parameters
- NVS storage failures
- RTC communication errors
- Feeding service failures
- Schedule conflict resolution

## Example Usage
```c
// Initialize scheduler
scheduler_service_init();

// Create morning feeding schedule
schedule_entry_t morning_feed = {
    .id = 1,
    .hour = 7,
    .minute = 30,
    .weekdays = SCHEDULER_DAILY,
    .enabled = true,
    .name = "Morning Feed"
};

// Add schedule
scheduler_service_add_schedule(&morning_feed);

// Enable scheduler
scheduler_service_enable(true);

// Main loop processing
while(1) {
    scheduler_service_process();  // Check for triggered alarms
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```