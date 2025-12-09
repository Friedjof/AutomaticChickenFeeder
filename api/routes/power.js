const express = require('express');
const router = express.Router();

// POST /api/power/sleep - shut down AP and enter deep sleep (simulated)
router.post('/sleep', (req, res) => {
    console.log('[API] POST /api/power/sleep - shutting down AP and entering deep sleep (simulated)');

    // In real firmware: stop WiFi/AP, then call esp_deep_sleep_start()
    res.json({
        success: true,
        message: 'Device entering deep sleep (simulated)',
        timestamp: new Date().toISOString()
    });
});

module.exports = router;
