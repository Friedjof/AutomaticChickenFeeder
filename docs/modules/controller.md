# System Controller

## Overview
The system controller is the central coordinator of the firmware. It instantiates every service, wires shared dependencies through a `ServiceContext`, and owns the main event loop. Its job is to translate wake reasons and external requests into a deterministic sequence of service calls while keeping each module isolated from low-level details it does not own.

## Service Interface Contract
All runtime modules implement a shared interface so the controller can treat them uniformly.
```cpp
struct ServiceContext {
    StorageService& storage;
    ScheduleService& schedule;
    ClockService& clock;
    FeedingService& feeding;
    PowerService& power;
    WebUiService& webUi;
    NotificationService* notification; // optional
    InputService* input;               // optional
    // plus shared utilities: logger, event bus, metrics, etc.
};

class IService {
public:
    virtual bool init(ServiceContext& ctx) = 0;    // one-off initialisation
    virtual void loop(ServiceContext& ctx) = 0;    // cooperative work unit, called each iteration
    virtual void onWake(ServiceContext& ctx) {};   // invoked after deep-sleep wake
    virtual void onSleep(ServiceContext& ctx) {};  // invoked before entering sleep
    virtual ~IService() = default;
};
```
Concrete services store their internal state and talk to peers only via references exposed in `ServiceContext`. This avoids global singletons and keeps dependencies explicit.

## Boot Sequence
1. **Reset reason detection**: Determine if the boot is cold reset, RTC wake, or external GPIO wake. This influences initial actions (e.g. captive portal activation).
2. **Construct services**: Instantiate Storage, Schedule, Clock, Feeding, Power, WebUi, optional Notification/Input, and collect them in an ordered list.
3. **Build context**: Create `ServiceContext ctx{ storage, schedule, clock, feeding, power, webUi, … }`.
4. **Initialise services**: Iterate over the list, calling `service->init(ctx)`. Recommended order:
   1. StorageService (mount LittleFS, open NVS, load defaults)
   2. ScheduleService (load schedules from storage)
   3. ClockService (I2C init, read current time)
   4. FeedingService (configure PWM/GPIO)
   5. PowerService (configure sleep sources, WiFi state)
   6. WebUiService (prepare web server, but keep WiFi off unless needed)
   7. Optional Notification/Input
   - On failure, controller enters safe mode (e.g. blink LED, keep AP on).
5. **Initial alarm decision**: Using ScheduleService + ClockService, compute the next feed time and call `clock.scheduleAlarm(nextTime)`. On cold boot also enable captive portal for user setup.

## Main Loop Structure
```cpp
void SystemController::run() {
    while (true) {
        for (auto* s : services_) {
            s->loop(ctx_);            // cooperative multitasking
        }
        processEvents();              // drains event queue (RTC alarms, UI commands, button actions)
        dispatchStateChanges();       // pushes updates to UI/notifications as needed
        if (sleepManager_.canSleep()) {
            prepareForSleep();
        }
        delay(kLoopDelayMs);         // e.g. 10-20 ms to avoid busy-wait
    }
}
```
- `loop()` implementations must be non-blocking; lengthy operations are split across iterations or handled via state machines.
- `processEvents()` handles cross-service messages: e.g. `AlarmFired`, `ManualFeedRequested`, `SessionStarted/Ended`.
- `dispatchStateChanges()` synchronises derived state (for Web UI snapshots, notifications).

## Sleep Workflow
1. Call `service->onSleep(ctx)` for each module (optional but useful for cleanup):
   - WebUiService stops HTTP server, requests WiFi shutdown via PowerService.
   - FeedingService powers down servo driver FETs.
   - NotificationService silences outputs.
2. Build `SystemSnapshot` (last feed time, grams dispensed, pending job id) and persist via `storage.saveSnapshot(snapshot)`.
3. Compute next job using `schedule.peekNextAfter()` or `nextDueJob()` if none in progress.
4. Program RTC alarm: `clock.scheduleAlarm(nextTime)`; ensure it is in the future.
5. Call `power.enterDeepSleep(wakeSources)`, enabling RTC alarm and button GPIOs as wake sources.

## Wake Handling
Upon resume, controller inspects reset reason:
- **Cold boot** (`POWERON_RESET`, `SW_RESET`): treat as fresh start → enable AP, load defaults, set alarm.
- **RTC alarm** (`ESP_SLEEP_WAKEUP_TIMER`):
  1. `clock.acknowledgeAlarm()`.
  2. `schedule.nextDueJob(now)` returns job; hand it to FeedingService.
  3. After feeding, compute `schedule.peekNextAfter()` and reprogram alarm.
- **Button wake** (`ESP_SLEEP_WAKEUP_EXT0/1`): InputService detects which button was pressed.
  - Feed button → run single-scoop feed cycle and return to sleep.
  - Settings button → enable WiFi/AP and keep the system awake for configuration until Web UI session ends.
- **Unknown**: enter safe mode, keep AP active, await user intervention.

Next, the controller calls `service->onWake(ctx)` on every module to let them refresh cached handles (e.g. re-open I2C, reapply LED states). Afterwards, it returns to `run()` main loop.

## Event Pipeline Examples
### RTC Alarm → Feed → Sleep
1. Alarm interrupt posts `AlarmFired` event.
2. `processEvents()` handles it: ack RTC, fetch job, call `feeding.start(job)`.
3. FeedingService drives servo via successive `loop()` calls until done, then signals `FeedCompleted` event.
4. Controller updates snapshot counters, asks ScheduleService for next occurrence, reprograms alarm, evaluates if it can sleep.

### Web UI Schedule Change
1. WebUiService receives PUT `/api/v1/schedules` with updated array.
2. Validates client payload, calls `storage.saveSchedules()`.
3. ScheduleService reloads data (or is given the new array) and recalculates next job.
4. Controller compares old vs. new upcoming job; if changed, reprograms `clock.scheduleAlarm()`.
5. UI gets updated snapshot via `dispatchStateChanges()`.

### Button Manual Feed
1. InputService detects a Feed button press; publishes `ManualFeedRequested`.
2. Controller instructs FeedingService to dispense the configured single scoop.
3. After completion, controller logs the event, updates snapshot counters, and immediately evaluates the sleep path.

### Settings Button → WiFi Session
1. InputService detects a Settings button press; publishes `EnableWifiRequested`.
2. Controller asks PowerService to start the SoftAP and prevents sleep while WebUiService serves the captive portal.
3. When the user clicks “Finish & Sleep”, WebUiService emits `SessionEnded`, allowing the controller to resume normal sleep preparation.

## Error Handling
- Service `init()` returning false triggers safe mode; controller keeps WiFi on, logs error, may blink LED.
- Runtime errors should be raised as events; controller can instruct NotificationService or fallback to safe mode depending on severity.
- Watchdog: PowerService or controller can arm a software watchdog inside the main loop (reset if loop delays exceed threshold).
- Structured logging/event bus helpers are still TODO—modules should surface TODO comments where they require richer diagnostics.

## Testing Strategy
- Mock each service (or use fake implementations) to test controller lifecycle: order of calls, response to events, sleep transitions.
- Simulate wake causes by injecting events (`AlarmFired`, `ButtonPressed`).
- Use host-based tests to ensure snapshot persistence and alarm reprogramming logic handle edge cases (e.g. alarm already in past).
