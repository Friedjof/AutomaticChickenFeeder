# Feeding Service

## Purpose
The FeedingService controls the dual-servo dispensing mechanism. It manages the complete feed cycle including power control, servo movement, timing sequences, and feed event tracking.

## Hardware
- **ESP32-C3** (Seeed XIAO ESP32C3)
- **Servo 1 & 2**: Controlled via ESP32Servo library
- **Transistor**: MOSFET for servo power management (GPIO 5)
- **DS3231 RTC**: For timestamp tracking

## Responsibilities
- Execute feed cycles with configurable portion counts (1-5 units)
- Manage servo power via transistor to minimize energy consumption
- Implement state machine for deterministic servo sequencing
- Track feed events with timestamps for history
- Provide feed status to other services

## State Machine

### States
1. **IDLE**: Servos powered down, waiting for feed request
2. **POWER_ON**: Transistor enabled, waiting for servo voltage stabilization
3. **ATTACH_SERVOS**: Servos attached to PWM pins
4. **SERVO_READY**: Servos ready, preparing for movement
5. **MOVING**: Servos executing open/close sequence
6. **DETACH_SERVOS**: Servos detached from PWM
7. **POWER_OFF**: Transistor disabled, servos unpowered
8. **FEED_WAITING**: Waiting between open/close cycles for multi-portion feeds

### Timing Constants
- `POWER_ON_DELAY`: 100ms - Servo power stabilization
- `SERVO_ATTACH_DELAY`: 100ms - Time after attach before movement
- `SERVO_MOVE_TIME`: 620ms - Duration for servo movement completion
- `FEED_WAIT_TIME`: 1000ms - Delay between open and close

## Feed Cycle Flow

### Single Portion
1. User/Timer triggers `feed(1)`
2. State: IDLE → POWER_ON (enable transistor)
3. State: POWER_ON → ATTACH_SERVOS (attach servos at 100ms)
4. State: ATTACH_SERVOS → SERVO_READY (servos write target position immediately)
5. State: SERVO_READY → MOVING (wait for stabilization)
6. State: MOVING → DETACH_SERVOS (after 620ms movement time)
7. State: DETACH_SERVOS → POWER_OFF (detach servos)
8. State: POWER_OFF → IDLE (disable transistor, record feed event)

### Multiple Portions
For `feed(count > 1)`, the cycle repeats:
- Open gate → Wait → Close gate → Wait → Open gate (next portion)
- After final portion, system returns to IDLE
- Single feed event recorded with total portion count

## Feed History Tracking

### Recording
- Every feed completion calls `recordFeedEvent()`
- Stores: Unix timestamp + portion_units
- Maintains ring buffer of last 10 feeds in RAM
- Persisted to NVS before deep sleep

### Storage
- Ring buffer: `FeedHistoryEntry feedHistory[10]`
- Tracked via: `feedHistoryCount` and `feedHistoryIndex`
- Circular overwrite of oldest entry when full

## Public API

```cpp
void feed(uint8_t count = 1);              // Trigger feed with 1-5 portions
void update();                              // Must be called in loop()
bool isFeeding();                           // Check if feed in progress
uint8_t getPosition();                      // Get current servo position (0-180)
uint32_t getLastFeedTimestamp();            // Unix timestamp of last feed

// Feed history management
void addFeedToHistory(uint32_t timestamp, uint8_t portionUnits);
uint8_t getFeedHistoryCount() const;
const FeedHistoryEntry* getFeedHistory() const;
void loadFeedHistory(const FeedHistoryEntry* history, uint8_t count);
void clearFeedHistory();

void setClockService(ClockService* clock);  // Required for timestamps
```

## Servo Configuration
- **Servo1**: Inverted position `SERVO_MAX_ANGLE - targetPosition`
- **Servo2**: Direct position `targetPosition`
- **Range**: 0° (closed) to 180° (open)
- **PWM Range**: 500-2400μs

## Power Management
- Servos only powered during active feed cycles
- Transistor (GPIO 5) controls power rail
- Automatic power-off after each cycle completes
- Prevents unnecessary battery drain

## Integration Points

### ClockService
- Required for timestamp generation
- Set via `setClockService(ClockService*)` during setup
- Used by `recordFeedEvent()` to get current time

### ConfigService
- Provides portion unit grams configuration
- Loads/saves feed history to NVS

### SchedulingService
- Calls `feed(portionUnits)` when scheduled event triggers
- Monitors `isFeeding()` to prevent sleep during operation

### WebService
- Exposes feed history via `/api/status/history`
- Triggers manual feeds via `/api/feed` endpoint

## Safety Features
- Single feed request guard: ignores requests while feeding
- Portion count validation: clamped to 1-5 range
- State machine prevents power glitches
- Servo detach before power-off prevents brown-out

## Testing
- Manual feed via double-click button
- API endpoint: `POST /api/feed`
- Monitor serial output for state transitions
- Verify feed history via `GET /api/status/history?limit=10`
