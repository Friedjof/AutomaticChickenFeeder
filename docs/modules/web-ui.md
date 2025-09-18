# Web UI Module

## Purpose
The Web UI provides a lightweight configuration surface that is available only while a user is connected to the feeder's hotspot. By avoiding background connections and live updates, the interface keeps WiFi radio usage to a minimum and preserves battery life.

## Captive Portal Flow
- ESP32-C6 runs a temporary SoftAP; DNS redirection leads clients automatically to `http://feeder.local/` (captive portal experience).
- Landing screen summarises the current system status (next feed, last feed, battery level) and offers access to the scheduler editor.
- After a user-initiated timeout (e.g. "Done" button or inactivity timer) the UI asks the System Controller to shut down WiFi.

## Functional Requirements
- Configure up to 5 recurring feed schedules. Each entry stores:
  - Enable/disable toggle.
  - Day-of-week mask represented as a byte bitmask (UI still renders seven toggles).
  - Execution time (HH:MM).
  - Portion in scoop units (1..kMaxPortionUnits); the UI displays derived grams using the configured scoop weight (default 12 g).
- Manual feed trigger (1..kMaxPortionUnits scoops) with confirmation dialog.
- Display static telemetry snapshot loaded on page open (last feed time, next scheduled feed, cumulative feed today, RTC sync status, battery state).
- Settings button press activates WiFi; the UI should remind users to close the session so the controller can return to deep sleep.
- Browser clock synchronisation: script pushes the browser's current time to the device every 10 seconds while the page is open to keep the RTC aligned.
- No PIN/authentication on the page itself; access is controlled through the WiFi password.

## Interaction Pattern
- Page loads once, fetches state via REST and renders controls. No live updates or WebSocket connections.
- User actions (change schedule, trigger feed, close session) send REST calls and wait for success/failure responses.
- After a successful change the UI refreshes the local view by re-fetching relevant data.

## Pages & Components
- **Dashboard**: snapshot cards (next feed, last feed, battery), "Trigger Feed" button, link to scheduler.
- **Schedule Editor**: table with five rows. Each row: enable toggle, time picker, weekday selector, portion dropdown. "Apply" button sends the full schedule payload.
- **Session Controls**: button to end session and shut down WiFi, optional indicator for connection status.
- **Time Sync Indicator**: shows last RTC sync timestamp (updates every 10 seconds after sync call completes).

## Data Contracts
- `GET /api/v1/state` → returns snapshot for dashboard (next feed, last feed, battery, RTC drift, schedule summary, `portion_unit_grams`, `max_portion_units`).
- `GET /api/v1/schedules` → returns full five-entry schedule list.
- `PUT /api/v1/schedules` → accepts array of five schedule entries (with `portion_units` integers and `weekday_mask` byte bitmasks).
- `POST /api/v1/feed` → triggers manual feed (optional `portion_units` override).
- `POST /api/v1/rtc/sync` → body contains browser timestamp; device adjusts RTC if drift exceeds threshold.
- `POST /api/v1/session/end` → instructs System Controller to disable WiFi and exit captive portal mode.
- All payloads are JSON. Scheduler entry example:
```
{
  "id": 1,
  "enabled": true,
  "time": "06:30",
  "portion_units": 2,
  "weekday_mask": 62
}
```
> `weekday_mask` uses bit0=Sunday .. bit6=Saturday (62 = Monday–Friday).

## Integration Points
- Uses `ScheduleService` for reading and applying schedule data.
- Sends manual feed requests to `FeedingService` through the System Controller.
- Pulls telemetry snapshot from `TelemetryService` / `SystemController`.
- Invokes `ClockService` via REST endpoint to align RTC with browser clock.
- Requests `PowerService` to shut down WiFi when the session ends.

## Power & UX Considerations
- WiFi stays off except during explicit sessions; UI should remind users to close the session when finished.
- Avoid large assets; total UI payload should stay small (<150 kB) for quick captive portal load.
- Provide clear confirmation messages for successful saves and manual feeds.
- Because there is no push channel, display the timestamp of the last snapshot so users know when data was fetched.

- Host static assets from LittleFS (`data/` directory copied from `data-template/`) via AsyncWebServer.
- Vanilla JS or a micro-framework keeps build artefacts minimal. Add a simple polling loop only for the RTC sync POST every 10 seconds.
- Derive gram labels client-side (`scoop_count * portion_unit_grams`) so schedule payloads stay in units.
- Fetch `portion_unit_grams` from the state endpoint on load and reuse it for all gram calculations.
- Use the schedule metadata (kMaxPortionUnits) provided by firmware constants to bound dropdown values; default UI can pre-render 1..kMaxPortionUnits.
- Guard the RTC sync endpoint with a drift threshold to avoid unnecessary writes (e.g. adjust only if drift > ±3 seconds).

## User Stories

### Story 1 – Quick Status & Manual Control Visit
As a farmer, I want to connect to the feeder hotspot, review the feeder status, and optionally trigger a manual feed so that I can leave the coop confident everything is on track.
When I open the captive portal, the dashboard shows the countdown to the next feeding, the last feed timestamp, and the battery level without any additional refresh steps.
If I notice something off, I can press the manual feed button, confirm the action, and watch the status card update once the dispense completes. The dialog lets me pick how many scoops to run, while showing the corresponding weight for clarity.
After the visit, I end the session with “Finish and Sleep,” ensuring the hotspot powers down to conserve energy.

**Steps**
- Connect to the feeder hotspot from a phone or tablet while at the coop.
- Let the captive portal redirect to the dashboard and review the status cards.
- If needed, tap “Manual Feed,” choose the number of scoops, confirm, and monitor the confirmation message.
- Observe the updated last-feed time to verify the action succeeded.
- Tap “Finish and Sleep” to close the session and reduce power usage.

### Story 2 – Seasonal Schedule Adjustment & Time Alignment
As a farmer, I want to tweak the weekly feeding schedule and implicitly keep the RTC aligned so that the flock still receives meals at the right times when routines change.
Within the schedule editor I adjust weekdays, shift times, and choose the number of scoops (with the UI showing the resulting grams), confident that the browser supplies accurate time data to the device every ten seconds.
After submitting the changes, I expect the UI to validate inputs, confirm success, and display the revised plan so I know the configuration is stored correctly.
Because the running time sync keeps the RTC in step with my device clock, the new schedule remains reliable without manual time-setting.

**Steps**
- Open the scheduler editor from the dashboard navigation menu.
- Modify the relevant slot: toggle active days, adjust HH:MM, pick the scoop count (grams shown alongside).
- Review all five entries to ensure they reflect the intended weekly pattern.
- Submit the schedule; wait for the confirmation toast and refreshed snapshot.
- Keep the page open briefly so the auto-sync can correct any RTC drift before closing.

### Story 3 – Controlled Session Shutdown & Power Saving
As a farmer, I want to close the configuration session explicitly and see WiFi power down so that the feeder returns to its low-consumption state without lingering connections.
The interface warns me about unsaved changes, submits any pending updates, and notifies the System Controller to disable WiFi once the session ends.
If I walk away without pressing the button, an inactivity timer reminds me that the portal will sleep soon, preventing accidental battery drain.
When I confirm the shutdown, the UI acknowledges success, letting me disconnect from the hotspot knowing the device is back in idle mode.

**Steps**
- Finish any configuration or manual actions on the dashboard or schedule editor.
- Tap “Finish and Sleep” to initiate session shutdown.
- Confirm save prompts so the device persists pending changes.
- Wait for the message that WiFi is turning off, then disconnect from the hotspot.
- Reconnect via the captive portal in the future whenever configuration is required.
