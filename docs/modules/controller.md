# Main Controller (main.cpp)

## Purpose
The main.cpp file coordinates all services, manages the application lifecycle, handles wake/sleep logic, and implements the main loop.

## Architecture
Simple procedural design with direct service instantiation:
- No abstract interfaces or event bus
- Services communicate via direct method calls
- State managed through function calls and flags

## Services

### Instantiation
```cpp
ButtonService buttonService;
FeedingService feedingService;
ClockService clockService;
ConfigService configService;
SchedulingService schedulingService(configService, clockService, feedingService);
WebService webService(configService, clockService, feedingService, schedulingService);
```

### Initialization Order (setup())
1. **ConfigService**: Initialize NVS, load settings
2. **ClockService**: Initialize RTC, check time validity
3. **Feed History**: Load from NVS into FeedingService
4. **ButtonService**: Setup GPIO, attach handlers
5. **FeedingService**: Configure servo pins, set clock reference
6. **SchedulingService**: Generate events, program first alarm
7. **WebService**: Prepare server (doesn't start until AP active)

## Hardware Configuration

### Pin Definitions
```cpp
#define RTC_INT_PIN 3      // DS3231 alarm interrupt
#define BUTTON_PIN 4       // User button
#define SERVO1_PIN 21      // Dispenser servo 1
#define SERVO2_PIN 2       // Dispenser servo 2
#define TRANSISTOR_PIN 5   // Servo power control
```

### Wake Sources
- **RTC Alarm**: GPIO 3 (scheduled feeds)
- **Button**: GPIO 4 (manual control, AP activation)

## State Flags

```cpp
bool wokeFromRtcAlarm = false;   // Woke from scheduled alarm
bool wokeFromButton = false;      // Woke from button press
unsigned long lastActivityMillis; // Last user/system activity
unsigned long ignoreButtonUntil;  // Button debounce after wake
```

## Boot Sequence

### Wake Cause Detection
```cpp
esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
uint64_t gpioStatus = esp_sleep_get_gpio_wakeup_status();
```

**Wake Reasons:**
- `ESP_SLEEP_WAKEUP_GPIO`: RTC alarm or button
- Other: Power-on reset or unknown

### Maintenance Mode
- Hold button during boot
- Logged as `[MAINTENANCE]`
- Features:
  - Persistent AP (no timeout)
  - OTA updates enabled
  - No automatic sleep
  - Infinite loop serving web UI

### Normal Boot Flow
1. Detect wake cause
2. Initialize all services
3. Load feed history
4. Handle wake-specific actions:
   - **RTC Alarm**: Process due schedules immediately
   - **Button**: Start AP after 2s delay
5. Enter main loop

## Main Loop

```cpp
void loop() {
    buttonService.loop();
    feedingService.update();
    webService.update();
    schedulingService.update();
    handleSleepLogic();
}
```

### Service Updates
- **ButtonService**: Poll button state, trigger callbacks
- **FeedingService**: Advance state machine, complete feeds
- **WebService**: Process DNS, check AP timeout
- **SchedulingService**: Check RTC alarm flag

### Sleep Logic
```cpp
void handleSleepLogic()
```

**Conditions preventing sleep:**
- Feeding in progress
- AP active
- Recent activity (< 2 minutes)

**Automatic sleep triggers:**
- RTC alarm completed (return to sleep immediately)
- Inactivity timeout (2 minutes)

## Button Handlers

### Single Click
```cpp
void simpleClickHandler(Button2 &btn)
```

- Starts AP mode
- Resets activity timer
- 2-second ignore window after wake

### Double Click
```cpp
void doubleClickHandler(Button2 &btn)
```

- Triggers manual feed (1 portion)
- Resets activity timer
- 2-second ignore window after wake

### Long Press
```cpp
void longClickHandler(Button2 &btn)
```

- Enters deep sleep immediately
- No ignore window (always active)

## Deep Sleep Management

### Entering Sleep
```cpp
void enterDeepSleep(const char* reason)
```

1. Save feed history to NVS
2. Stop AP if active
3. Configure wake sources (RTC + Button)
4. Enable GPIO wake on LOW signal
5. Log reason and enter deep sleep

**Wake Mask:**
```cpp
const uint64_t wakeMask = (1ULL << RTC_INT_PIN) | (1ULL << BUTTON_PIN);
esp_deep_sleep_enable_gpio_wakeup(wakeMask, ESP_GPIO_WAKEUP_GPIO_LOW);
```

### Sleep Triggers
- "RTC alarm handled" (after feed completed)
- "Inactivity timeout" (2 minutes idle)
- "Manual long press" (user requested)
- "Remote request" (via WebService)

## Activity Tracking

### Mark Activity
```cpp
void markActivity()
```

- Updates `lastActivityMillis`
- Called on:
  - Button presses
  - Feeding operations
  - Web UI interactions (via WebService)

### Inactivity Timeout
- `INACTIVITY_SLEEP_MS`: 120000 (2 minutes)
- Compared against max of `lastActivityMillis` and web client activity
- Triggers automatic sleep

## Feed History Persistence

### Load (setup)
```cpp
FeedHistoryEntry history[MAX_FEED_HISTORY];
uint8_t historyCount = configService.loadFeedHistory(history, MAX_FEED_HISTORY);
feedingService.loadFeedHistory(history, historyCount);
```

### Save (before sleep)
```cpp
configService.saveFeedHistory(feedingService.getFeedHistory(),
                               feedingService.getFeedHistoryCount());
```

## Power Management Strategy

### Active Time Minimization
- Sleep between scheduled feeds
- AP only active when needed
- Servo power only during dispense
- WiFi disabled in sleep

### Wake/Sleep Cycle
1. **Wake** (RTC or button)
2. **Execute** (feed or serve UI)
3. **Sleep** (automatic after completion)

Typical cycle: <30 seconds active, hours sleeping

## Error Handling

### Service Init Failures
- Logged to serial with `[ERROR]` prefix
- System continues with reduced functionality
- Example: RTC unavailable → no scheduled feeds, manual still works

### Watchdog
- No explicit watchdog (relies on ESP32 default)
- Feeding timeout prevents servo lockup
- AP timeout prevents battery drain

## Logging

All major events logged to serial (115200 baud):
- `[INFO]`: Normal operations
- `[WARN]`: Recoverable issues
- `[ERROR]`: Critical failures
- `[DEBUG]`: Detailed state info
- Service-specific: `[BUTTON]`, `[SCHED]`, `[WEB]`, `[CLOCK]`, `[CONFIG]`

## Testing

### Boot Testing
1. Power on → cold boot
2. Hold button → maintenance mode
3. RTC wake → alarm execution
4. Button wake → AP activation

### Sleep Testing
1. Manual feed → auto sleep after 2 min
2. Long press → immediate sleep
3. RTC alarm → execute and return to sleep
4. AP timeout → auto sleep after 60s

### Integration Testing
1. Schedule feed → verify RTC alarm → wake and feed
2. Manual feed → verify history saved
3. Config change → verify alarm reprogrammed
4. Factory reset → verify defaults restored
