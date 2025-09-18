# Input Service

## Purpose
The input service handles user buttons or other physical inputs. It debounces signals, recognises gestures (short/long press, combinations), and emits high-level events for the System Controller to act upon (manual feeding, factory reset, WiFi wake).

## Responsibilities
- Configure GPIOs with interrupts or polling as appropriate for the hardware.
- Debounce button states and recognise simple presses (no long/double press logic required).
- Map buttons to actions:
  - **Feed button** → request a single scoop dispense.
  - **Settings button** → enable WiFi SoftAP and keep the controller awake for configuration.
- Expose current input state (e.g. `feedPressed`, `settingsPressed`) for diagnostics.
- Optionally integrate with NotificationService to provide feedback (short beep on press).

## Operation
1. `init(ctx)` sets up GPIO direction, pull-ups, interrupt handlers.
2. `loop(ctx)` samples state (if polling) or processes queued interrupts; updates internal state machine.
3. On button press, publish events (`ManualFeedRequested`, `EnableWifiRequested`) via controller for subsequent handling.
4. Optional: additional maintenance combo can be added later for factory reset.

## Integration
- **SystemController**: receives events such as `ManualFeedRequested`, `OpenWifiSession`, `FactoryReset`. Handles them in `processEvents()`.
- **PowerService**: ensures that, when button triggers manual feed, servo power and WiFi state follow the requested action.
- **WebUiService**: may be notified to start captive portal when user requests configuration mode.

## Safety
- Prevent button chatter from triggering repeated feeds by enforcing cooldowns.
- Provide audible/visual feedback to confirm button detection (optional via NotificationService).
- Avoid triggering feed if Feeding module reports it is currently busy.

## Testing
- Simulate button press sequences with mocked GPIO layer to verify gesture recognition.
- Ensure manual feed request leads to a single feed event despite long button press.
- Verify that gestures are ignored during feeding if that is a design requirement (optional lockout).
