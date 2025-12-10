# Button Service (Input)

## Purpose
The ButtonService handles the single physical button interface, providing debounced input detection and multi-gesture recognition (single click, double click, long press).

## Hardware
- **Button Pin**: GPIO 4
- **Type**: Active-LOW with internal pull-up
- **Library**: Button2 (gesture recognition)

## Responsibilities
- Initialize button GPIO with pull-up
- Detect button gestures (single, double, long press)
- Provide callback registration for main application
- Handle button debouncing automatically
- Support wake-from-sleep button handling

## Button Gestures

### Single Click
- **Action**: Start WiFi Access Point
- **Use Case**: Open web UI for configuration
- **Callback**: `setSimpleClickHandler()`
- **Debounce**: 2-second ignore window after wake

### Double Click
- **Action**: Trigger manual feed (1 portion)
- **Use Case**: Quick feeding without UI
- **Callback**: `setDoubleClickHandler()`
- **Debounce**: 2-second ignore window after wake

### Long Press
- **Action**: Enter deep sleep immediately
- **Use Case**: Manual power down
- **Callback**: `setLongClickHandler()`
- **No Debounce**: Always active (override safety)

## Public API

```cpp
void begin();                                         // Initialize button
void loop();                                          // Must be called in loop()
void setSimpleClickHandler(void (*callback)(Button2&));
void setDoubleClickHandler(void (*callback)(Button2&));
void setLongClickHandler(void (*callback)(Button2&));
```

## Initialization

```cpp
void ButtonService::begin()
```

1. Configure Button2 library
2. Set GPIO 4 as input with pull-up
3. Register click handlers
4. Log initialization

## Main Loop

```cpp
void ButtonService::loop()
```

- Calls `button.loop()` from Button2 library
- Handles debouncing and gesture detection
- Triggers registered callbacks on events

## Wake Handling

### Wake Detection
In main.cpp:
```cpp
if (wokeFromButton) {
    ignoreButtonUntil = millis() + 2000;  // 2-second ignore window
}
```

### Button Ignore Logic
```cpp
if (millis() < ignoreButtonUntil) {
    Serial.println("[BUTTON] Ignoring click (too soon after wakeup)");
    return;  // But still mark activity
}
```

**Purpose**: Prevent consuming the wakeup button press as an action

## Callback Implementations (main.cpp)

### Single Click Handler
```cpp
void simpleClickHandler(Button2 &btn) {
    markActivity();
    if (millis() < ignoreButtonUntil) return;

    Serial.println("[BUTTON] Single click - Starting AP mode");
    webService.startAP("ChickenFeeder", "");
}
```

### Double Click Handler
```cpp
void doubleClickHandler(Button2 &btn) {
    markActivity();
    if (millis() < ignoreButtonUntil) return;

    Serial.println("[BUTTON] Double click - Manual feed");
    feedingService.feed(1);
}
```

### Long Press Handler
```cpp
void longClickHandler(Button2 &btn) {
    Serial.println("[BUTTON] Long click - Entering deep sleep");
    enterDeepSleep("Manual long press");
}
```

**Note**: Long press has no ignore window - always processes

## Maintenance Mode Detection

In main.cpp setup():
```cpp
pinMode(BUTTON_PIN, INPUT_PULLUP);
delay(50);  // Debounce
bool maintenanceModeRequested = (digitalRead(BUTTON_PIN) == LOW);
```

If button held during boot:
- Enter maintenance mode
- Persistent AP with OTA enabled
- No automatic sleep
- Infinite loop serving web UI

## Integration Points

### Main Application
- Registers callback handlers during setup
- Implements ignore window logic
- Calls `loop()` in main loop
- Marks activity on button events

### WebService
- Started via single-click callback
- AP mode activated for configuration

### FeedingService
- Triggered via double-click callback
- Executes 1-portion feed

### Power Management
- Long press triggers immediate deep sleep
- Button wake prevents timeout sleep
- Activity tracking resets sleep timer

## Button2 Library Features

### Automatic Handling
- Hardware debouncing
- Click counting (single/double)
- Long press timing
- Event callbacks

### Timing Thresholds
- Click timeout: ~400ms
- Long press threshold: ~1000ms
- Double click window: ~400ms

(Library defaults, not explicitly configured)

## Wake Source Configuration

In main.cpp before sleep:
```cpp
const uint64_t wakeMask = (1ULL << RTC_INT_PIN) | (1ULL << BUTTON_PIN);
esp_deep_sleep_enable_gpio_wakeup(wakeMask, ESP_GPIO_WAKEUP_GPIO_LOW);
```

**Wake Trigger**: LOW signal (button pressed)

## Safety Features

### Click Debouncing
- Hardware debounce via Button2
- Software ignore window after wake
- Prevents accidental double-triggers

### Activity Marking
- All button events mark activity
- Resets sleep timeout
- Keeps system awake during interaction

### Long Press Override
- No ignore window on long press
- Allows emergency sleep from any state
- User can force sleep if system stuck

## Testing

### Gesture Testing
1. Single click → verify AP starts
2. Double click → verify feed triggers
3. Long press → verify immediate sleep
4. Test ignore window: press immediately after wake

### Wake Testing
1. Sleep device
2. Press button → verify wake
3. Verify 2-second ignore window active
4. Test that third press triggers action

### Maintenance Mode
1. Hold button during power-on
2. Verify persistent AP starts
3. Check maintenance mode logs

## Logging
All button events logged with `[BUTTON]` prefix:
- Click type (single/double/long)
- Action taken
- Ignore window status
- Activity marking
