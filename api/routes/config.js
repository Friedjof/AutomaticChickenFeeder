const express = require('express');
const router = express.Router();
const configService = require('../services/config');

// GET /api/config - Get current configuration
router.get('/', (req, res) => {
    try {
        const config = configService.getConfig();
        
        console.log('[API] GET /api/config');
        
        res.json({
            success: true,
            data: config,
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Config request failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// POST /api/config - Update configuration
router.post('/', (req, res) => {
    try {
        const updates = req.body;
        
        console.log('[API] POST /api/config', JSON.stringify(updates, null, 2));
        
        const result = configService.updateConfig(updates);
        
        if (result.success) {
            const updatedConfig = configService.getConfig();
            res.json({
                success: true,
                message: 'Configuration updated successfully',
                data: updatedConfig,
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
        console.error('[API] Config update failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// POST /api/config/reset - Reset to default configuration
router.post('/reset', (req, res) => {
    try {
        console.log('[API] POST /api/config/reset');
        
        configService.resetToDefaults();
        const config = configService.getConfig();
        
        res.json({
            success: true,
            message: 'Configuration reset to defaults',
            data: config,
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Config reset failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

// GET /api/config/schedules - Get human-readable schedule info
router.get('/schedules', (req, res) => {
    try {
        const scheduleInfo = configService.getScheduleInfo();
        
        console.log('[API] GET /api/config/schedules');
        
        res.json({
            success: true,
            data: {
                schedules: scheduleInfo,
                activeSchedules: configService.getActiveSchedules()
            },
            timestamp: new Date().toISOString()
        });
        
    } catch (error) {
        console.error('[API] Schedules request failed:', error.message);
        res.status(500).json({
            success: false,
            error: 'Internal server error',
            timestamp: new Date().toISOString()
        });
    }
});

module.exports = router;
