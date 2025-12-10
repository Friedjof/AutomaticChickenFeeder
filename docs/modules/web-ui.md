# Web Service & UI

## Purpose
The WebService provides a captive portal interface for configuration and manual control. It hosts a responsive web UI and REST API that runs only when the WiFi Access Point is active.

## Architecture
- **AsyncWebServer**: ESPAsyncWebServer library
- **DNSServer**: Captive portal DNS redirect
- **Static Assets**: Embedded in firmware via `generated/web_files.h`
- **AP Mode Only**: WiFi disabled during normal operation

## Hardware
- **ESP32-C3** WiFi radio
- **AP IP**: 192.168.4.1
- **DNS Port**: 53

## Responsibilities
- Host captive portal Access Point
- Serve static web UI assets (HTML, CSS, JS)
- Provide REST API for config and control
- Handle OTA firmware updates
- Manage AP lifecycle and timeouts

## Access Point Management

### Starting AP
```cpp
void startAP(const char* ssid = "ChickenFeeder", const char* password = "");
```

Triggered by:
- Button single-click (GPIO 4)
- Wake from button during sleep
- Maintenance mode (button held during boot)

### AP Timeout Logic
- **No client connected**: 60 seconds timeout
- **Client connected**: Stays active indefinitely
- **Inactivity**: User must disconnect to trigger sleep

### Stopping AP
```cpp
void stopAP();
```

Automatic shutdown:
- Timeout expired
- Sleep requested
- Maintenance mode exited

## REST API Endpoints

### Status & Telemetry
**GET** `/api/status`
```json
{
  "success": true,
  "data": {
    "isOnline": true,
    "isFeeding": false,
    "servoPosition": "Closed",
    "lastFeedTime": "2025-01-15T14:30:00Z",
    "totalFedToday": 0
  }
}
```

**GET** `/api/status/history?limit=10`
```json
{
  "success": true,
  "data": {
    "feeds": [
      {
        "timestamp": "2025-01-15T14:30:00Z",
        "portion": 36
      }
    ]
  }
}
```

### Configuration
**GET** `/api/config`
```json
{
  "success": true,
  "data": {
    "version": 1,
    "portion_unit_grams": 12,
    "schedules": [
      {
        "id": 1,
        "enabled": true,
        "time": "06:30",
        "weekday_mask": 62,
        "portion_units": 2
      }
    ]
  }
}
```

**POST** `/api/config` (Content-Type: application/json)
```json
{
  "schedules": [
    {
      "id": 1,
      "enabled": true,
      "time": "06:30",
      "weekday_mask": 62,
      "portion_units": 2
    }
  ],
  "portion_unit_grams": 12
}
```

### Manual Feed
**POST** `/api/feed`
```json
{
  "success": true,
  "message": "Feed cycle started"
}
```

### Time Sync
**POST** `/api/time` (Content-Type: application/json)
```json
{
  "timestamp": "2025-01-15T14:30:00Z"
}
```

Response:
```json
{
  "success": true,
  "message": "RTC synchronized"
}
```

### Power Management
**POST** `/api/power/sleep`
```json
{
  "success": true,
  "message": "Entering sleep mode"
}
```

### Factory Reset
**POST** `/api/config/reset`
```json
{
  "success": true,
  "message": "Configuration reset to defaults"
}
```

### OTA Updates
**GET** `/api/ota/status`
```json
{
  "success": true,
  "version": "2.0.3",
  "available": true
}
```

**POST** `/api/ota/update` (multipart/form-data)
- Upload `.bin` firmware file
- Performs flash update
- Reboots on success

## Web UI Features

### Dashboard
- System status cards
- Last feed timestamp
- Next scheduled feed
- Manual feed button
- Schedule management link

### Schedule Editor
- 6 configurable schedule slots
- Time picker (HH:MM)
- Weekday selector (7 toggles)
- Portion units dropdown (1-5)
- Enable/disable toggle per slot
- Save button triggers `/api/config` POST

### Feed History (NEW)
- Last 10 feed events
- Timestamp display
- Portion size in grams
- Collapsible on mobile

### Settings
- Portion unit grams configuration
- Factory reset button
- OTA firmware upload
- "Take a Nap" (sleep) button

## Captive Portal

### DNS Redirection
All DNS queries redirect to 192.168.4.1:
- Android: `/generate_204`
- iOS: `/hotspot-detect.html`
- Windows: `/connecttest.txt`

All redirect to `/` (index.html)

### Static File Serving
Assets embedded in firmware:
- `index.html`: Main UI
- `app.js`: Application logic
- `style.css`: Responsive styles
- `mock/api.js`: Offline development mock

Files served with proper MIME types via AsyncWebServer.

## Maintenance Mode

### Activation
Hold button (GPIO 4) during boot to enter maintenance mode.

### Features
- Persistent AP (no timeout)
- Open WiFi (no password): SSID "ChickenFeeder"
- OTA updates enabled
- No automatic sleep
- Serial logging: `[MAINTENANCE]` prefix

### Exit
Power cycle or firmware upload required.

## Integration Points

### ConfigService
- Loads/saves schedules
- Provides portion unit grams
- Handles factory reset

### SchedulingService
- Calls `onConfigChanged()` after config save
- Triggers event regeneration and alarm update

### FeedingService
- Triggers manual feeds
- Checks `isFeeding()` status
- Retrieves last feed timestamp
- Provides feed history data

### ClockService
- Syncs RTC from browser time
- Formats timestamps for API responses

### Main Application
- Receives sleep callback via `setSleepCallback()`
- Monitors AP activity via `isAPActive()`
- Tracks client activity via `getLastClientActivity()`

## Security
- **No authentication**: Physical access required (connect to AP)
- **Optional password**: Can set WiFi password in `startAP()`
- **Local only**: No internet connectivity
- **Temporary**: AP times out automatically

## Client Activity Tracking
```cpp
uint32_t getLastClientActivity() const;
```

Updated on every API request via `updateClientActivity()`.
Used by main application to prevent sleep during active sessions.

## Error Handling
- Invalid JSON: 400 Bad Request
- Feed while feeding: 400 with error message
- Portion validation: Rejects <1 or >5 units
- Serial logging: `[WEB]` prefix for all operations

## Testing
1. Connect to "ChickenFeeder" WiFi
2. Browser should auto-open captive portal
3. Manual: Navigate to http://192.168.4.1
4. Test all API endpoints via UI
5. Monitor serial output for `[WEB]` logs
6. Verify AP timeout after 60s with no client
