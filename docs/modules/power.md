# Power Service

## Purpose
The power service manages the device’s power modes. It coordinates WiFi activation, servo power rails, and deep-sleep transitions so the feeder runs on minimal energy while still waking for scheduled tasks or user interactions.

## Responsibilities
- Configure sleep sources (RTC alarm, button GPIOs) and enter/exit deep sleep on demand.
- Switch WiFi SoftAP on/off based on controller or Web UI requests.
- Control servo power enable pins/FETs to reduce standby consumption.
- Track battery/voltage readings if hardware provides sensors (optional integration with NotificationService).
- Expose status flags (`wifiActive`, `servoPowerActive`, `sleepPending`) for diagnostics.

## Key APIs
- `enableWifiAp()` / `disableWifiAp()` – toggles SoftAP; internally uses ESP-IDF WiFi APIs and coordinate with WebUiService.
- `setServoPower(bool on)` – drives GPIO controlling servo rail.
- `prepareDeepSleep(WakeSources sources)` – configures wake-up triggers.
- `enterDeepSleep()` – final call before MCU sleeps; ensures WiFi and unnecessary peripherals are off.
- `onWake()` – reinitialise power domains if needed (e.g. re-enable I2C power).

## Integration
- **SystemController**: instructs PowerService to enter deep sleep when conditions met; queries ability to sleep (e.g. `canSleep()` or checks whether WiFi session active).
- **WebUiService**: requests WiFi AP when user opens captive portal; notifies when session ends.
- **FeedingService**: asks for servo power while dispensing.
- **NotificationService**: may need temporary power during alerts (short beeps/LEDs).
- **StorageService**: optionally logs energy usage or battery metrics.

## Workflow Example
1. Cold boot → PowerService enables WiFi AP so user can configure schedule.
2. Settings button press → InputService raises `EnableWifiRequested`; PowerService turns on WiFi and keeps the controller awake until WebUiService signals `SessionEnded`.
3. Feed button press → PowerService keeps WiFi off, but ensures servo rail is powered for FeedingService.
4. Before sleep → SystemController calls `power.prepareDeepSleep()` with RTC alarm + button wake sources → `power.enterDeepSleep()`.
5. On RTC wake → `power.onWake()` re-enables servo rail only when FeedingService requests it.

## Testing
- Mock `esp_sleep_*` and WiFi calls to ensure functions are invoked with correct parameters.
- Unit test that servo power toggles align with feeding lifecycle (Power off when idle).
- Validate that enabling AP sets `sleepPending` false to prevent premature sleep.
