# Config Service (Storage & Persistence)

## Purpose
The ConfigService manages all persistent configuration using ESP32's NVS (Non-Volatile Storage). It provides a simple key-value interface for schedules, settings, and feed history that survives deep sleep and power cycles.

## Hardware Platform
- **ESP32-C3** (Seeed XIAO ESP32C3)
- **NVS Partition**: Flash-based key-value storage
- **Namespace**: "feeder"

## Storage Medium
Uses ESP32 `Preferences` library (NVS wrapper):
- Key-value pairs stored in flash
- Survives deep sleep and power cycles
- Wear-leveling built into NVS
- No filesystem overhead (LittleFS not used)

## Responsibilities
- Load and save feed schedules (6 slots)
- Manage portion unit configuration (grams per unit)
- Persist feed history (last 10 feeds)
- Provide factory reset functionality
- Ensure data integrity across power cycles

## Data Structures

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

### Feed History Entry
```cpp
struct FeedHistoryEntry {
    uint32_t timestamp;      // Unix timestamp
    uint8_t portion_units;   // Total portions fed
};
```

## Configuration Parameters

### Constants
- `MAX_SCHEDULES`: 6 feed schedule slots
- `MAX_FEED_HISTORY`: 10 historical feed entries
- Default portion size: 12g per unit

### NVS Keys
| Key | Type | Description |
|-----|------|-------------|
| `sched_0` ... `sched_5` | String (JSON) | Individual schedule entries |
| `portionGrams` | UChar | Grams per portion unit (default 12) |
| `feedHist` | Bytes | Binary blob of feed history |
| `feedHistCnt` | UChar | Number of valid history entries |

## Public API

```cpp
// Lifecycle
bool begin();  // Initialize NVS, load defaults

// Schedule management
bool loadSchedule(uint8_t index, Schedule &schedule);
bool saveSchedule(uint8_t index, const Schedule &schedule);
bool loadAllSchedules(Schedule schedules[MAX_SCHEDULES]);
bool saveAllSchedules(const Schedule schedules[MAX_SCHEDULES]);

// Portion configuration
uint8_t getPortionUnitGrams();
void setPortionUnitGrams(uint8_t grams);

// Feed history
bool saveFeedHistory(const FeedHistoryEntry* history, uint8_t count);
uint8_t loadFeedHistory(FeedHistoryEntry* history, uint8_t maxCount);
bool clearFeedHistory();

// Factory reset
bool resetToDefaults();
```

## Schedule Storage Format

Schedules are stored as JSON strings for flexibility:
```json
{
  "id": 1,
  "enabled": true,
  "time": "06:30",
  "weekday_mask": 62,
  "portion_units": 2
}
```

**Weekday Mask Encoding:**
- Bit 0 = Sunday
- Bit 1 = Monday
- ...
- Bit 6 = Saturday
- Example: 62 (0b0111110) = Monday–Friday

## Feed History Storage

Stored as binary blob for efficiency:
- Array of `FeedHistoryEntry` structs
- Size: `count * sizeof(FeedHistoryEntry)` bytes
- Separate counter stores number of valid entries
- Loaded at startup, saved before deep sleep

## Default Configuration

Factory defaults (applied on first boot or reset):
```cpp
Schedule defaults[6] = {
    {1, false, "06:30", 62, 1},  // Mon-Fri 6:30am, 1 unit
    {2, false, "12:00", 62, 1},  // Mon-Fri 12:00pm, 1 unit
    {3, false, "18:00", 62, 1},  // Mon-Fri 6:00pm, 1 unit
    {4, false, "22:00", 62, 1},  // Mon-Fri 10:00pm, 1 unit
    {5, false, "00:00", 0, 1},   // Disabled
    {6, false, "00:00", 0, 1}    // Disabled
};
portion_unit_grams = 12;
```

## Integration Points

### Main Application
- `begin()` called during setup to initialize NVS
- Feed history loaded at startup, saved before sleep

### SchedulingService
- Loads schedules on initialization
- Calls `saveAllSchedules()` after API updates
- Triggers schedule regeneration after changes

### FeedingService
- Queries `getPortionUnitGrams()` for gram calculations
- Provides feed history data for persistence

### WebService
- API calls trigger schedule saves
- Returns portion unit grams in status responses
- Serves feed history via `/api/status/history`

## Error Handling
- Missing keys return default values
- Invalid JSON triggers default schedule creation
- NVS failures logged to serial, operation continues with defaults
- `begin()` returns false only on critical NVS init failure

## Data Validation
- Portion units clamped to 1-5 range
- Time format validated (HH:MM)
- Weekday mask within valid range (0-127)
- Schedule index bounds checked (0-5)

## Flash Endurance
- NVS uses wear-leveling automatically
- Schedules saved only on explicit API calls
- Feed history saved once per sleep cycle
- Typical writes: <10 per day under normal usage
- NVS flash blocks rated for 10,000+ cycles

## Factory Reset
Triggered via `resetToDefaults()`:
1. Restore default schedules (all disabled except presets)
2. Reset portion unit grams to 12g
3. Clear all feed history
4. Logs reset completion to serial

## Testing
- Serial output shows NVS operations
- Monitor `[CONFIG]` log prefix for debug info
- Verify persistence across power cycles
- Test factory reset via API or serial command
