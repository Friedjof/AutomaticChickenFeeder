# Notification Module (Optional)

The notification module centralises how the feeder communicates alerts to the outside world. It is considered optional because the initial PlatformIO port can run without user-facing buzzers or LEDs, yet the architecture leaves space for future signalling features.

## Purpose
- Provide a single service interface for raising alerts (feeding errors, low battery, RTC issues, maintenance reminders).
- Abstract specific output hardware such as status LEDs, buzzers, or small displays so other services stay decoupled.
- Offer escalation logic (e.g. repeated beeps for unresolved faults) and quiet hours or acknowledgement handling once input buttons confirm an issue.

## Integration
- Receives events from the System Controller (e.g. `FeedCycleStarted`, `FeedCycleFailed`).
- Coordinates with the Power module to ensure notification hardware is powered only when needed.
- Optional bridge to the Web UI for surfacing alerts in the captive portal during active sessions.

## Future Work
- Define concrete output patterns (blink/beep sequences) per event severity.
- Add configuration hooks so farmers can mute or adjust notification intensity.
- Consider logging notifications to storage for troubleshooting once persistent telemetry is added.
