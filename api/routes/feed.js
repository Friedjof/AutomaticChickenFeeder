const express = require('express');
const router = express.Router();
const servoService = require('../services/servo');
const configService = require('../services/config');

// POST /api/feed - Trigger manual feed
router.post('/', async (req, res) => {
    try {
        const config = configService.getConfig();
        const portionSize = req.body.portion || config.portion_unit_grams;
        
        console.log(`[API] POST /api/feed (portion: ${portionSize}g)`);
        
        // Validate portion size
        if (portionSize < 1 || portionSize > 100) {
            return res.status(400).json({
                success: false,
                error: 'Invalid portion size. Must be between 1-100 grams.',
                timestamp: new Date().toISOString()
            });
        }
        
        const result = await servoService.triggerFeed(portionSize);
        
        if (result.success) {
            res.json({
                success: true,
                message: result.message,
                data: {
                    portion: result.portion,
                    feedTime: new Date().toISOString()
                },
                timestamp: new Date().toISOString()
            });
        } else {
            res.status(400).json({
                success: false,
                error: result.error,
                timestamp: new Date().toISOString()
            });
        }
        
    } catch (error) {
        console.error('[API] Feed request failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// GET /api/feed/status - Get feeding status (alternative to /api/status)
router.get('/status', (req, res) => {
    try {
        const status = servoService.getStatus();
        
        console.log('[API] GET /api/feed/status');
        
        res.json({
            success: true,
            data: {
                isFeeding: status.isFeeding,
                servoPosition: status.servoPosition,
                canFeed: !status.isFeeding
            },
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Feed status request failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// POST /api/feed/stop - Emergency stop (simulate hardware interrupt)
router.post('/stop', (req, res) => {
    try {
        console.log('[API] POST /api/feed/stop - Emergency stop');
        
        // In real ESP32, this would stop the servo immediately
        // For simulation, we'll just reset the state
        servoService.isFeeding = false;
        servoService.position = 'closed';
        
        res.json({
            success: true,
            message: 'Feed operation stopped',
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Feed stop failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// POST /api/feed/test - Test servo without dispensing (diagnostic)
router.post('/test', async (req, res) => {
    try {
        console.log('[API] POST /api/feed/test - Servo test mode');
        
        if (servoService.isFeeding) {
            return res.status(400).json({
                success: false,
                error: 'Cannot test while feeding is active',
                timestamp: new Date().toISOString()
            });
        }
        
        // Simulate a quick servo test without recording feed
        const originalPosition = servoService.position;
        
        servoService.position = 'opening';
        await servoService.delay(200);
        servoService.position = 'open';
        await servoService.delay(500);
        servoService.position = 'closing';
        await servoService.delay(200);
        servoService.position = 'closed';
        
        res.json({
            success: true,
            message: 'Servo test completed successfully',
            data: {
                testDuration: '900ms',
                servoPosition: 'closed'
            },
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Servo test failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Servo test failed: ' + error.message,
            timestamp: new Date().toISOString()
        });
    }
});

module.exports = router;
