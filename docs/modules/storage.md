# Storage Module (Configuration & Persistence)

The storage module combines configuration handling and general persistence management into a single service. It mediates between high-level modules (Web UI, Schedule, Power, System Controller) and the limited-write flash/NVS resources of the ESP32-C6, ensuring that structured data is validated, debounced, and committed safely.

## Configuration Layers

### Fixed Definitions (compile-time)
These values live in header files (e.g. `include/config/hardware_map.hpp`) or `constexpr` structures compiled into the firmware. They rarely change per device and protect flash endurance by avoiding writes.

| Category | Examples | Notes |
| --- | --- | --- |
| Hardware map | `SERVO_DISPENSE_PIN`, `SERVO_GATE_PIN`, `RTC_SDA_PIN`, `BUTTON1_PIN`, `BUTTON2_PIN` | Sourced from board design; used by drivers/hal. |
| WiFi fallbacks | `kDefaultApSsid`, `kDefaultApPass`, `kWifiChannel` | Provide deterministic captive portal defaults for factory reset; SSID/password remain compile-time constants. |
| Timing constants | `kButtonHoldMs`, `kManualFeedTimeoutMs`, `kWifiBootGraceMs` | Ensure coherent behaviour across services. |
| Schedule limits | `kScheduleSlots = 5`, `kPortionUnitGrams = 12`, `kMaxPortionUnits = 10` | Referenced by Web UI validation and Schedule Service. |
| File & namespace IDs | `kConfigPath = "/config.json"`, `kSnapshotNamespace = "state"` | Prevents typos across modules. |
| Migration version | `kConfigSchemaVersion = 3` | Bumped when layout changes; stored alongside JSON. |
| Default fallback blobs | `kDefaultConfigJson` (PROGMEM) | Copied to LittleFS during factory reset. |

### Persistent JSON Configuration (LittleFS)
The JSON document captures user-adjustable settings. Version-controlled defaults live in `data-template/config.json`; copy that file to the working `data/config.json` before flashing. The document is written sparingly and validated against the fixed definitions above. Example structure:

```json
{
  "version": 3,
  "portion_unit_grams": 12,
  "schedules": [
    { "id": 1, "enabled": true, "time": "06:30", "weekday_mask": 62, "portion_units": 2 },
    { "id": 2, "enabled": false, "time": "12:00", "weekday_mask": 62, "portion_units": 3 },
    { "id": 3, "enabled": false, "time": "18:00", "weekday_mask": 62, "portion_units": 2 },
    { "id": 4, "enabled": false, "time": "00:00", "weekday_mask": 0, "portion_units": 1 },
    { "id": 5, "enabled": false, "time": "00:00", "weekday_mask": 0, "portion_units": 1 }
  ],
  "rtc": {
    "sync_threshold_ms": 3000
  }
}
```

Key sections:
- `portion_unit_grams`: optional override for the scoop size if the compiled default needs adjustment (not exposed in the captive portal by default).
- `schedules`: fixed-length (5) array constrained by `kScheduleSlots`; entries stored exactly as provided by the Web UI. `weekday_mask` is a single byte bitmask (bit0=Sunday … bit6=Saturday) and `portion_units` multiplies the compile-time scoop weight to derive grams.

### Runtime Snapshot (NVS / RTC RAM)
Ephemeral state is stored outside `config.json` to minimise flash wear. Values are compact key/value pairs:

| Key | Description | Medium |
| --- | --- | --- |
| `last_feed_time` | Unix timestamp of latest successful dispense | NVS |
| `dispensed_today_g` | Daily cumulative feed in grams | NVS |
| `pending_job_id` | Schedule entry awaiting execution after wake | NVS |
| `rtc_drift_ms` | Last measured correction applied during browser sync | NVS |
| `wake_hint` | One-byte flag to indicate RTC/button wake reason | RTC RAM |
| `ui_session_active` | Set while captive portal is engaged | RTC RAM |

Runtime values are updated via debounced transactions (see Flash Endurance section) and read back on boot to reconstruct state quickly.

## Responsibilities (High Level)
- Maintain the authoritative configuration (`config.json`) including schedule definitions and RTC sync policy while everything hardware-specific remains compile-time.
- Provide load/save APIs for high-level domain objects (schedules, RTC sync policy) used by the System Controller and Schedule module.
- Surface the effective scoop weight (`portion_unit_grams`) and the configured limit (`kMaxPortionUnits`) with fallback to compile-time defaults so views can render gram equivalents and bounds.
- Maintain short-lived runtime snapshots (last feed timestamp, pending job pointer) so the device can resume after deep sleep without re-computing schedules, throttling writes to protect flash endurance.
- Offer schema versioning and automatic migration routines to keep existing installations compatible with new firmware.
- Expose validation errors to the caller so that faulty input from the Web UI can be rejected before writing to flash.

## Storage Media Strategy (ESP32-C6 Constraints)
- **LittleFS (flash filesystem)**
  - Stores `config.json` authored by the Web UI.
  - Uses atomic update pattern: write to temporary file, validate, rename over the active file.
  - Suitable for data that changes infrequently (schedules, RTC policy).
- **NVS (key-value store)**
  - Holds small pieces of frequently updated data (e.g. last feed timestamp, RTC drift offset, portal counters).
  - Supports quick read/write without mounting the filesystem and survives deep sleep.
- **RTC Memory (optional)**
  - Caches minimal wake-up context such as "next job index" to speed up resume after light sleep. Data is considered advisory; authoritative copy remains in LittleFS/NVS.

## Data Model Overview
- `config.json`
  - `version`: integer schema version for migration logic.
  - `portion_unit_grams`: scoop size used to translate the unit count into grams; defaults to compile-time constant if absent.
  - `schedules`: array of five entries `{ id, enabled, time, weekday_mask, portion_units }` persisted exactly as the Web UI submits them.
  - `rtc`: policy for browser-driven synchronization (threshold in ms).
- `state.nvs`
  - `last_feed_time`: Unix timestamp of last successful dispense.
  - `total_dispensed_today`: cumulative grams dispensed since midnight.
  - `rtc_drift_offset`: latest measured adjustment when syncing with browser clock.
  - `pending_job_id`: identifier of the schedule entry to resume after sleep.
  - `ap_session_counter`: optional metric to track captive portal usage without touching LittleFS.

## Service API Sketch
```cpp
struct ScheduleEntry {
    uint8_t id;
    bool enabled;
    TimeOfDay time;
    uint8_t weekdayMask;   // bit 0 = Sunday, bit 6 = Saturday
    uint8_t portionUnits; // number of scoops (1..kMaxPortionUnits)
};

struct SystemSnapshot {
    time_t lastFeed;
    uint16_t gramsDispensedToday; // derived from portion_units * portion_unit_grams
    uint8_t pendingJobId;
    int32_t rtcDriftOffsetMs;
};

class StorageService {
public:
    bool loadConfig(DeviceConfig &outConfig);
    bool saveConfig(const DeviceConfig &config, ValidationReport &report);

    bool loadSchedules(std::array<ScheduleEntry, 5> &outSchedules);
    bool saveSchedules(const std::array<ScheduleEntry, 5> &schedules, ValidationReport &report);

    bool loadSnapshot(SystemSnapshot &outSnapshot);
    bool saveSnapshot(const SystemSnapshot &snapshot);

    bool factoryReset();
};
```
- `ValidationReport` aggregates field-level errors (invalid time format, portion units out of range, conflicting weekday mask).
- `factoryReset()` reverts to built-in defaults by deleting files and NVS keys when the user holds buttons during boot.

## Interaction with Other Modules
- **Web UI** calls `saveSchedules()` when the farmer updates the timetable and reads `portion_unit_grams` via the state endpoint to show derived weights. Errors propagate back so the UI can display validation messages.
- **Schedule module** loads the current schedule during boot and saves updates when automation logic adjusts next-run pointers.
- **Feeding module** consumes `portion_units` and the scoop weight to translate jobs into servo movements.
- **System Controller** requests `loadSnapshot()` during startup and `saveSnapshot()` before entering deep sleep.
- **Input module** can trigger `factoryReset()` when a maintenance button sequence is detected.

## Migration & Validation
- Keep an embedded `default_config.json` resource that the service can copy to LittleFS on first boot or after factory reset.
- Use schema `version` to run migrations (e.g. add new fields with defaults, rename keys). Each migration should be idempotent.
- Validate schedules:
  - Exactly `kScheduleSlots` items provided in the array.
  - `portion_units` between 0 and `kMaxPortionUnits` (must be ≥1 when the entry is enabled).
  - `time` matches HH:MM and is stored in minutes since midnight internally.
  - `weekday_mask` != 0 if entry is enabled.

## Flash Endurance & Power Considerations
- Batch writes and use write-back caching to minimise flash wear (LittleFS flash blocks typically tolerate ~10k write cycles). Apply differential updates instead of rewriting entire config files.
- Debounce snapshot commits (e.g. buffer updates and persist on sleep or after a 30 s inactivity window) to avoid exceeding flash write endurance.
- Gracefully handle filesystem mount failures by signalling `StorageUnavailable` to the System Controller, prompting a safe mode.

## Testing Notes
- Provide unit tests for JSON parsing/serialisation ensuring defaults load correctly and validation rejects malformed input (including scoop counts and weekday masks).
- Mock the filesystem when running on host to simulate write failures and ensure error paths are covered.
- Include migration tests per schema version to guard against regressions during future updates.
