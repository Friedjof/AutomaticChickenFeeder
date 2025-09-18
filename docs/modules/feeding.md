# Feeding Service

## Purpose
The feeding service drives the mechanical dispensing subsystem. It translates scheduled jobs (portion units, timing) into concrete servo movements, manages power to the actuators, and enforces safety checks to prevent jams or overfeeding.

## Responsibilities
- Accept feed requests from the System Controller (scheduled or manual) and execute them deterministically.
- Convert `portion_units` into servo commands using the compile-time scoop weight and optional calibration data.
- Drive the dispensing servo/gate sequence (e.g. open gate, rotate dispenser, close gate) with configurable timings.
- Monitor for failure conditions (e.g. motor stall, timeout) and signal errors back through events/NotificationService.
- Expose status for snapshots (e.g. `isFeeding`, `lastFeedResult`).

## Operation Flow
1. **Start**: `startFeed(Job job)` stores target portion units, resets timers, enables servo power via PowerService.
2. **Loop execution** (`loop(ctx)`): state machine steps through phases:
   - `Idle` → `Begin`: ensure servo power enabled, optionally wait for stable voltage.
   - `Dispense`: rotate servo for each scoop unit (counted pulses), using delay between steps.
   - `Gate`: if second servo exists, open/close gate before/after rotation.
   - `Done`: stop motors, disable power, emit completion event.
3. **Completion**: update snapshot counters (`gramsDispensedToday += units * portion_unit_grams`, `lastFeedTime = now`).
4. **Error handling**: if servo does not reach expected position or exceeds timeout, raise `FeedFailed` event, optionally retry or enter safe mode.

## Interactions
- **SystemController**: issues `start()` commands after schedule resolution; listens for completion/failure callbacks.
- **PowerService**: requested to keep servo rail powered only while feeding; ensures minimal energy use.
- **StorageService**: provides calibration data (if needed) via config JSON, and records feed outcomes in snapshot.
- **NotificationService** (optional): beep/LED on success/failure.
- **WebUiService**: displays feed status through snapshots (pulled from Storage/SystemController).

## Inputs & Outputs
- Inputs: `FeedJob` structure (job ID, portion units, scheduled time). Manual feed uses similar struct with `origin = manual`.
- Outputs: events (`FeedStarted`, `FeedCompleted`, `FeedFailed`) dispatched through controller’s event bus; snapshot updates.

## Safety Considerations
- Servo timeouts; abort if cycle takes longer than configured limit.
- Stall detection (e.g. by monitoring servo current or using feedback switch, if available).
- Debounce manual feed requests to avoid repeated triggers.
- Ensure gate closes before sleeping to avoid open dispenser.

## Testing
- Use mock servo drivers to simulate rotation counts and confirm state machine transitions.
- Verify correct gram calculation (`portion_units * portion_unit_grams`).
- Simulate errors (timeout/stall) to confirm events propagate and power shuts down.
