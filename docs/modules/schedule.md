# Schedule Module

## Purpose
The schedule module owns the feeding timetable logic. It stores the five recurring slots, resolves which job is due next based on the current time, and exposes query/update operations without touching hardware. This separation keeps the business rules easy to test and reuse while other services focus on RTC interaction or servo control.

## Responsibilities
- Persist and expose the fixed-length schedule (5 entries) containing `enabled`, `time`, `weekday_mask` (byte bitmask), and `portion_units`.
- Provide deterministic queries:
  - `nextDueJob(DateTime now)` → returns the next active slot with occurrence timestamp and portion units.
  - `peekNextAfter(JobId id, DateTime now)` → calculates the following occurrence after a given job completed.
  - `listActiveJobs()` → enumerates enabled entries for UI/state snapshots.
- Offer mutation APIs used by the Web UI or automated logic (`updateSchedule(array<ScheduleEntry,5>)`). Validation is handled by StorageService; Schedule module assumes inputs are valid.
- Maintain lightweight runtime state such as `lastJobId` or cached iteration order to accelerate lookups.

## Interactions
- Reads/writes data through `StorageService` (load at boot, persist on change).
- Consumed by `SystemController` when deciding which job to execute on wake-up.
- Supplies summaries to `WebUiService` for rendering schedules, using scoop counts and the compile-time `portion_unit_grams`/`kMaxPortionUnits` metadata provided by Storage.

## Key Behaviours
- Computes next occurrence by comparing current weekday/time against each enabled slot; wraps across week boundaries when needed.
- Sorts jobs virtually without mutating user order (tie-breaking by slot index).
- Handles disabled slots gracefully by skipping them during iteration.
- Exposes utilities to convert the byte `weekday_mask` into friendly lists for the UI layer (optional helper).

## Testing Suggestions
- Unit test edge cases (wrap from Saturday night to Monday morning, multiple slots at identical times, disabled entries).
- Verify that `portion_units` flows through untouched so Feeding module can multiply by scoop weight.
- Mock StorageService to simulate load/save cycles and ensure state consistency after modifications.
