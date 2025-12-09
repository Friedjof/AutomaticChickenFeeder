const express = require('express');
const router = express.Router();
const servoService = require('../services/servo');

// GET /api/status - Get current device status
router.get('/', (req, res) => {
    try {
        const status = servoService.getStatus();
        
        // Log the request (like ESP32 serial output would show)
        console.log('[API] GET /api/status');
        
        res.json({
            success: true,
            data: status,
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Status request failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// GET /api/status/history - Get feeding history
router.get('/history', (req, res) => {
    try {
        const limit = parseInt(req.query.limit) || 10;
        const history = servoService.getFeedHistory(limit);
        
        console.log(`[API] GET /api/status/history (limit: ${limit})`);
        
        res.json({
            success: true,
            data: {
                feeds: history,
                totalFeeds: history.length
            },
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] History request failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

module.exports = router;
