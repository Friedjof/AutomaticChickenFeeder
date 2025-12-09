const express = require('express');
const cors = require('cors');

// Import services (initialize them)
const configService = require('./services/config');
const servoService = require('./services/servo');

// Import routes
const statusRoutes = require('./routes/status');
const configRoutes = require('./routes/config');
const feedRoutes = require('./routes/feed');
const powerRoutes = require('./routes/power');

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors({
    origin: true, // echo request origin (works for localhost, LAN, AP)
    credentials: true
}));
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Request logging middleware (simulates ESP32 serial output)
app.use((req, res, next) => {
    const timestamp = new Date().toISOString();
    console.log(`[${timestamp}] ${req.method} ${req.url}`);
    next();
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({
        success: true,
        message: 'ESP32-C3 Chicken Feeder API is running',
        uptime: process.uptime(),
        timestamp: new Date().toISOString(),
        version: '1.0.0'
    });
});

// API Routes
app.use('/api/status', statusRoutes);
app.use('/api/config', configRoutes);
app.use('/api/feed', feedRoutes);
app.use('/api/power', powerRoutes);

// Root endpoint - device info (like ESP32 would show)
app.get('/', (req, res) => {
    res.json({
        device: 'ESP32-C3 Chicken Feeder',
        version: '1.0.0',
        api_version: 'v1',
        endpoints: {
            status: '/api/status',
            config: '/api/config',
            feed: '/api/feed',
            power: '/api/power',
            health: '/health'
        },
        uptime_seconds: Math.floor(process.uptime()),
        memory_usage: process.memoryUsage(),
        timestamp: new Date().toISOString()
    });
});

// Error handling middleware
app.use((err, req, res, next) => {
    console.error('[ERROR]', err.message);
    res.status(500).json({
        success: false,
        error: 'Internal server error',
        timestamp: new Date().toISOString()
    });
});

// 404 handler
app.use((req, res) => {
    res.status(404).json({
        success: false,
        error: 'Endpoint not found',
        available_endpoints: [
            '/api/status',
            '/api/config',
            '/api/feed',
            '/api/power',
            '/health'
        ],
        timestamp: new Date().toISOString()
    });
});

// Start server
app.listen(PORT, '0.0.0.0', () => {
    console.log('');
    console.log('=================================');
    console.log('🐔 ESP32-C3 Chicken Feeder API');
    console.log('=================================');
    console.log(`Server running on port ${PORT}`);
    console.log(`Health check: http://localhost:${PORT}/health`);
    console.log(`API Base URL: http://localhost:${PORT}/api`);
    console.log('');
    console.log('Available endpoints:');
    console.log('  GET  /api/status        - Device status');
    console.log('  GET  /api/config        - Configuration');
    console.log('  POST /api/config        - Update config');
    console.log('  POST /api/config/reset  - Reset to defaults');
    console.log('  POST /api/feed          - Manual feed');
    console.log('  POST /api/feed/test     - Test servo');
    console.log('  POST /api/feed/stop     - Emergency stop');
    console.log('  POST /api/power/sleep   - Shutdown AP and enter deep sleep');
    console.log('');
    console.log('CORS: dynamic origin allowed (works on AP/LAN/localhost)');
    console.log('[ConfigService] Initialized with default configuration');
    console.log('[ServoService] Hardware simulation ready');
    console.log('=================================');
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n[System] Graceful shutdown initiated...');
    console.log('[ConfigService] Configuration saved to memory');
    console.log('[ServoService] Servo positioned to closed state');
    console.log('[System] ESP32-C3 Chicken Feeder API stopped');
    process.exit(0);
});

// Daily reset scheduler (like ESP32 would do at midnight)
setInterval(() => {
    const now = new Date();
    if (now.getHours() === 0 && now.getMinutes() === 0) {
        console.log('[System] Daily reset - clearing feed statistics');
        servoService.resetDailyStats();
    }
}, 60000); // Check every minute

module.exports = app;
