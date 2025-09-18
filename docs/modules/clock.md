# Clock Module

## Purpose
The clock module encapsulates all real-time clock (RTC) interactions. It abstracts the ESP32-C6 + DS3231 hardware, providing current time, alarm scheduling, and drift management. It never decides *which* feeding event should occur; it simply executes time-related commands issued by higher-level logic.

## Responsibilities
- Initialise and maintain the RTC connection (I2C bus setup, availability checks).
- Provide `now()` returning the authoritative current time (UTC or configured local offset).
- Configure exactly one hardware alarm at a time via `scheduleAlarm(DateTime when)`.
- Handle alarm acknowledgements (`acknowledgeAlarm()`), clearing RTC flags and reporting wake reason to the System Controller.
- Expose drift/synchronisation helpers:
  - Apply browser-provided timestamps (`applySync(timestamp, thresholdMs)`).
  - Report drift statistics for telemetry/UI.
- Offer low-level utilities such as `setTime(DateTime)`, `enableSqwOutput(bool)`, or `sleepUntilAlarm()` wrappers if needed by PowerService.

## Interactions
- Receives the next alarm timestamp from `SystemController` (computed using Schedule module).
- Notifies `SystemController` when an alarm interrupt fires so the feeding pipeline can continue.
- Persists drift metrics/sync data through `StorageService` (e.g. `rtc_drift_offset`).
- Coordinates with `PowerService` to ensure the MCU enters deep/light sleep with RTC wake-up enabled.

## Key Behaviours
- Guarantees there is at most one active RTC alarm; reprograms it immediately after each feed cycle based on new instructions from `SystemController`.
- Converts `DateTime` inputs to RTC register values, handling day rollovers and 24h format.
- Validates requested alarm times (e.g. not in the past) and adjusts for minimum lead time if required.
- Provides hooks for testing (mockable interface that simulates alarm callbacks).

## Testing Suggestions
- Implement hardware-abstraction interfaces so unit tests can run against a fake RTC (verifying conversions of time → alarm registers).
- Check drift correction logic by feeding timestamps within/outside the threshold.
- Ensure alarm reprogramming covers edge cases (wrap to next week, multiple consecutive alarms, manual time setting).
