# Web Interface Development Guide

## Overview

This guide explains how to develop and deploy the web interface for the ESP32-C3 Chicken Feeder. The project uses a dual-environment approach: Docker for development and ESP32 LittleFS for production deployment.

## Architecture

### Development Environment (`/web/` directory)
- **Purpose**: Full-featured development with live API simulation
- **Technology**: Docker Compose with Nginx reverse proxy + Node.js API backend
- **Features**: Hot reload, detailed logging, CORS handling, comprehensive API simulation
- **Access**: `http://localhost` (Port 80)

### Production Environment (`/data-template/` directory)
- **Purpose**: Optimized files for ESP32 deployment via PlatformIO
- **Technology**: Static files served by ESP32 AsyncWebServer from LittleFS
- **Constraints**: <150kB total size, minimal dependencies, captive portal compatible
- **Access**: `http://feeder.local/` (ESP32 SoftAP)

## File Structure

```
web/                          # Development Environment
├── index.html               # Main interface
├── css/style.css           # Responsive styling
├── js/app.js              # Frontend application logic
├── compose.yml            # Docker setup (legacy)
├── assets/icons/          # Icons and media
└── mock/api.js           # Mock API (deprecated)

data-template/             # Production Template
└── config.json          # ESP32 configuration file

api/                      # Development API Backend
├── server.js            # Express server
├── package.json         # Dependencies
├── routes/             # API route handlers
└── services/          # Business logic simulation
```

## Development Setup

### Prerequisites
- Docker and Docker Compose
- Node.js 18+ (for local development)
- Modern web browser with developer tools

### Quick Start
```bash
# Start development environment
docker compose up -d

# Access web interface
open http://localhost

# View API directly
curl http://localhost/api/status

# Stop environment
docker compose down
```

### Development Workflow

#### 1. Local Development
```bash
# Edit files in /web/ directory
code web/index.html
code web/css/style.css 
code web/js/app.js

# Changes are immediately reflected (Docker volume mount)
# Refresh browser to see updates
```

#### 2. API Development & Testing
The development environment includes a full ESP32 API simulation:

```bash
# Test API endpoints
curl http://localhost/api/status
curl http://localhost/api/config
curl -X POST http://localhost/api/feed

# View backend logs
docker compose logs -f api
```

#### 3. Frontend Development
- **API Base URL**: Use `/api` (relative paths work via reverse proxy)
- **Error Handling**: Network errors show "Offline" status
- **Real-time Updates**: 2-second polling for device status
- **Responsive Design**: Works on desktop, tablet, and mobile

## API Integration

### Current API Endpoints (Development Simulation)
```
GET  /api/status        # Device status & servo position
GET  /api/config        # Configuration & schedules  
POST /api/config        # Update configuration
POST /api/config/reset  # Reset to defaults
POST /api/feed          # Manual feed trigger
POST /api/feed/test     # Servo test (dev only)
```

### ESP32 Production API (Target Implementation)
Based on `docs/modules/web-ui.md`, the ESP32 will implement:
```
GET  /api/v1/state           # System snapshot
GET  /api/v1/schedules       # Feed schedules
PUT  /api/v1/schedules       # Update schedules  
POST /api/v1/feed           # Manual feed
POST /api/v1/rtc/sync       # Clock synchronization
POST /api/v1/session/end    # Close captive portal
```

### Data Contracts

#### Status Response
```json
{
  "success": true,
  "data": {
    "isOnline": true,
    "isFeeding": false,
    "servoPosition": "Closed",
    "lastFeedTime": "2024-10-29T18:30:00Z",
    "totalFedToday": 24,
    "batteryLevel": 85
  }
}
```

#### Configuration Response  
```json
{
  "success": true,
  "data": {
    "portion_unit_grams": 12,
    "schedules": [
      {
        "id": 0,
        "enabled": true,
        "time": "06:30", 
        "weekday_mask": 62,
        "portion_units": 2
      }
    ]
  }
}
```

## ESP32 Deployment

### Automatic Deployment Script
```bash
# Deploy web files to ESP32 data directory
make deploy-web

# Flash ESP32 with updated web interface
make flash

# Deploy and flash in one command  
make deploy-flash
```

### Manual Deployment
```bash
# Run deployment script directly
./scripts/deploy-web.sh

# Upload to ESP32 filesystem
pio run --target uploadfs
```

### File Size Optimization

The deployment script automatically:
- **Minifies CSS/JS**: Removes whitespace and comments
- **Compresses Images**: Optimizes PNG/JPG assets  
- **Removes Dev Files**: Excludes Docker, mock data, development tools
- **Validates Size**: Ensures total payload < 150kB

## Development Best Practices

### 1. ESP32 Constraints
- **Memory Limits**: Keep total web assets under 150kB
- **No External Dependencies**: Avoid CDN links, use local files only
- **Vanilla JavaScript**: No heavy frameworks (React, Vue, etc.)
- **Minimal CSS**: Use efficient selectors, avoid large animations

### 2. Captive Portal Compatibility  
- **Simple HTML Structure**: Avoid complex redirects
- **Fast Load Times**: Minimize initial page size
- **Clear Navigation**: Users expect simple, obvious controls
- **Session Management**: Provide clear "Finish and Sleep" option

### 3. Battery Optimization
- **Minimal Network Calls**: Batch requests where possible
- **No Background Polling**: Only update when user is active
- **Efficient UI Updates**: Use document fragments for DOM updates
- **Clear Session End**: Remind users to close the interface

### 4. Error Handling
- **Offline Mode**: Handle network disconnections gracefully
- **User Feedback**: Clear success/error messages for all actions
- **Input Validation**: Validate on both client and server side
- **Timeout Handling**: Handle slow ESP32 responses

## Testing

### 1. Development Testing
```bash
# Start development environment
docker compose up -d

# Test all functionality
open http://localhost

# Check browser console for errors
# Verify API calls in Network tab
# Test responsive design on mobile
```

### 2. ESP32 Testing
```bash
# Deploy to ESP32
make deploy-flash

# Connect to ESP32 hotspot
# Access http://feeder.local/

# Test captive portal behavior
# Verify all API endpoints work
# Check memory usage and performance
```

### 3. Cross-Browser Testing
- **Chrome/Chromium**: Primary development browser  
- **Firefox**: Alternative rendering engine
- **Safari Mobile**: iOS device compatibility
- **Chrome Mobile**: Android device compatibility

## Troubleshooting

### Common Development Issues

#### Docker Container Won't Start
```bash
# Check port conflicts
docker ps | grep 80

# Remove conflicting containers
docker compose down --remove-orphans
docker system prune -f
```

#### API Calls Fail
```bash
# Check nginx proxy logs
docker compose logs proxy

# Verify API backend is running
docker compose logs api

# Test API directly
curl http://localhost/api/status
```

#### File Changes Not Reflected
```bash
# Hard refresh browser (Ctrl+F5)
# Clear browser cache
# Check Docker volume mounts
docker compose exec proxy ls -la /usr/share/nginx/html/
```

### ESP32 Deployment Issues

#### Upload Fails
```bash
# Check ESP32 connection
make list

# Verify data directory exists  
ls -la data-template/

# Check file sizes
du -sh data-template/*
```

#### Web Interface Doesn't Load
```bash
# Check ESP32 serial output
make monitor

# Verify filesystem upload
# Check for memory issues in logs
# Test with minimal HTML file first
```

## Integration with ESP32 Code

### WebService Integration
The ESP32 `WebService` class should:
- Serve files from LittleFS `/data` partition
- Handle API routes via AsyncWebServer
- Implement captive portal DNS redirection
- Manage WiFi SoftAP lifecycle

### Configuration Sync
- Web interface reads/writes via `/api/v1/schedules`
- Changes persist to ESP32 flash memory
- Configuration validates against hardware limits
- Real-time clock sync maintains schedule accuracy

### Power Management
- Web interface reminds users to close sessions
- `POST /api/v1/session/end` triggers WiFi shutdown
- Inactivity timer prevents battery drain
- Deep sleep mode activated after session end

## Future Enhancements

### Progressive Web App (PWA)
- Add service worker for offline capability
- Implement app manifest for "Add to Home Screen"
- Cache static assets for faster loading

### Advanced Features  
- Feed history charts and analytics
- Weather integration for seasonal adjustments
- Push notifications for feed alerts
- Multi-language support

### Performance Optimization
- Implement lazy loading for non-critical assets
- Add compression middleware for API responses  
- Use WebP images for better compression
- Implement client-side caching strategies

## Resources

- **ESP32 Documentation**: [ESP32-C3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf)
- **AsyncWebServer Library**: [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer)
- **LittleFS Guide**: [ESP32 LittleFS](https://github.com/lorol/LITTLEFS)
- **Captive Portal Implementation**: [ESP32 Captive Portal](https://github.com/CDFER/Captive-Portal-ESP32)
