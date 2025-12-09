# Chicken Feeder Web Interface

This directory contains the web interface for the ESP32-C3 Chicken Feeder project. The interface is designed as a Single Page Application (SPA) that can be developed independently and later integrated into the ESP32 firmware.

## Development Setup

### Prerequisites
- Docker and Docker Compose
- Web browser

### Quick Start

1. **Start the development server:**
   ```bash
   cd web/
   docker compose up
   ```

2. **Open the interface:**
   Navigate to `http://localhost:8000` in your browser

3. **Development workflow:**
   - Edit files in the `web/` directory
   - Changes are immediately visible (live reload via nginx volume mount)
   - No need to restart the container

## Project Structure

```
web/
├── compose.yml          # Docker Compose configuration
├── index.html           # Main SPA file
├── css/
│   └── style.css        # Responsive styling
├── js/
│   └── app.js           # Main application logic
├── mock/
│   └── api.js           # Mock ESP32 API for development
└── assets/
    └── icons/           # Future icons/images
```

## Features

### Current Implementation
- **Responsive Design**: Works on desktop and mobile devices
- **Mock API**: Simulates ESP32 responses using localStorage
- **Real-time Updates**: Status refreshes every 2 seconds
- **Manual Feed Control**: Large, accessible feed button
- **Schedule Configuration**: 5 configurable feeding times with weekday selection
- **Portion Size Setting**: Adjustable feed portion (1-100g)
- **Visual Feedback**: Animations and toast notifications

### User Interface
- **Dashboard**: Current status, manual feed, last feed time
- **Status Section**: Servo position, portion size, total fed today
- **Configuration**: Portion settings and feeding schedules
- **Toast Notifications**: Success/error feedback

## Mock API

The `mock/api.js` provides a complete simulation of the ESP32 API:

### Available Endpoints (simulated)
- `getStatus()` - Device status and servo position
- `getConfig()` - Current configuration
- `saveConfig(config)` - Save new configuration
- `triggerFeed()` - Start manual feed cycle
- `resetConfig()` - Reset to defaults

### Data Persistence
Configuration is stored in browser localStorage, simulating ESP32 SPIFFS/LittleFS storage.

## Configuration Format

The interface uses the same JSON structure as the ESP32 firmware:

```json
{
  "version": 1,
  "portion_unit_grams": 12,
  "schedules": [
    {
      "id": 1,
      "enabled": true,
      "time": "06:30",
      "weekday_mask": 62,
      "portion_units": 1
    }
  ]
}
```

### Weekday Mask
- Bit 0 = Sunday, Bit 1 = Monday, ..., Bit 6 = Saturday
- Example: `62` = `0111110` = Monday through Friday

## Development Notes

### Styling
- **CSS Grid/Flexbox**: Responsive layout
- **CSS Variables**: Easy theming
- **Animations**: Smooth transitions and feed button feedback
- **Mobile-first**: Optimized for touch interfaces

### JavaScript Architecture
- **Class-based**: Clean, modular code structure
- **Event-driven**: Proper separation of concerns
- **Async/await**: Modern promise handling
- **Error handling**: Comprehensive error states

### Browser Compatibility
- Modern browsers (ES2017+)
- Mobile Safari, Chrome, Firefox
- No external dependencies

## Integration with ESP32

When ready to integrate with the ESP32:

1. **Replace Mock API**: Update `app.js` to use `fetch()` calls to ESP32 endpoints
2. **Deploy Files**: Copy HTML/CSS/JS to ESP32 SPIFFS/LittleFS
3. **Update Paths**: Ensure correct file paths for ESP32 web server
4. **Test Integration**: Verify API compatibility

### Expected ESP32 API Endpoints
```
GET  /api/status        # Device status
GET  /api/config        # Current configuration  
POST /api/config        # Save configuration
POST /api/feed          # Trigger manual feed
POST /api/reset         # Reset configuration
```

## Customization

### Theming
Edit CSS variables in `style.css`:
```css
:root {
  --primary-color: #667eea;
  --accent-color: #ff6b6b;
  --background-gradient: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
```

### Features
- Add new configuration options in `config-section`
- Extend mock API for additional endpoints
- Add more status indicators or charts

## Troubleshooting

### Container Issues
```bash
# Stop and remove container
docker compose down

# Rebuild and start
docker compose up --build
```

### Port Conflicts
If port 8000 is busy, edit `compose.yml`:
```yaml
ports:
  - "8080:80"  # Use port 8080 instead
```

### Browser Cache
Force refresh with `Ctrl+F5` or disable cache in developer tools.

## Future Enhancements

- **Feeding History**: Chart of daily/weekly feeding patterns
- **Camera Integration**: Live view of feeder area
- **WiFi Settings**: Configure network credentials
- **OTA Updates**: Firmware update interface
- **Scheduling Calendar**: Visual schedule management
- **Push Notifications**: Feed alerts and status updates
