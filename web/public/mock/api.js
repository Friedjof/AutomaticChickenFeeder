// Mock API for Chicken Feeder Development
// Simulates ESP32 API responses

class MockAPI {
    constructor() {
        // Load initial config from localStorage or use defaults
        this.config = this.loadConfig();
        this.status = {
            isOnline: true,
            servoPosition: 'Closed',
            lastFeedTime: null,
            totalFedToday: 0,
            isFeeding: false
        };
        
        // Simulate some previous feeds for demo
        const now = new Date();
        this.status.lastFeedTime = new Date(now.getTime() - 2 * 60 * 60 * 1000); // 2 hours ago
        this.status.totalFedToday = 36; // 3 previous feeds of 12g each
    }

    loadConfig() {
        const saved = localStorage.getItem('chicken-feeder-config');
        if (saved) {
            return JSON.parse(saved);
        }
        
        // Default config based on data-template/config.json
        return {
            version: 1,
            portion_unit_grams: 12,
            schedules: [
                { id: 1, enabled: true, time: "06:30", weekday_mask: 62, portion_units: 1 },
                { id: 2, enabled: true, time: "12:00", weekday_mask: 62, portion_units: 1 },
                { id: 3, enabled: true, time: "18:00", weekday_mask: 62, portion_units: 1 },
                { id: 4, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 5, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 }
            ],
            rtc: {
                sync_threshold_ms: 3000
            }
        };
    }

    saveConfig() {
        localStorage.setItem('chicken-feeder-config', JSON.stringify(this.config));
    }

    // Simulate network delay
    async delay(ms = 200 + Math.random() * 300) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    // API Endpoints
    async getStatus() {
        await this.delay();
        return {
            success: true,
            data: { ...this.status }
        };
    }

    async getConfig() {
        await this.delay();
        return {
            success: true,
            data: { ...this.config }
        };
    }

    async saveConfig(newConfig) {
        await this.delay();

        // Validate schedules if provided
        if (newConfig.schedules) {
            for (const schedule of newConfig.schedules) {
                if (schedule.portion_units < 1 || schedule.portion_units > 5) {
                    return {
                        success: false,
                        error: 'Invalid portion size. Must be between 1-5 units (12-60g).'
                    };
                }
            }
        }

        this.config = { ...this.config, ...newConfig };
        this.saveConfig();

        return {
            success: true,
            message: 'Configuration saved successfully'
        };
    }

    async triggerFeed() {
        await this.delay(100);
        
        if (this.status.isFeeding) {
            return {
                success: false,
                error: 'Feeder is already active'
            };
        }

        // Start feeding simulation
        this.status.isFeeding = true;
        this.status.servoPosition = 'Opening';
        
        // Simulate feeding process
        setTimeout(() => {
            this.status.servoPosition = 'Open';
        }, 500);
        
        setTimeout(() => {
            this.status.servoPosition = 'Closing';
        }, 1500);
        
        setTimeout(() => {
            this.status.servoPosition = 'Closed';
            this.status.isFeeding = false;
            this.status.lastFeedTime = new Date();
            this.status.totalFedToday += this.config.portion_unit_grams;
        }, 2000);

        return {
            success: true,
            message: 'Feed cycle started'
        };
    }

    async resetConfig() {
        await this.delay();

        // Reset to defaults
        this.config = {
            version: 1,
            portion_unit_grams: 12,
            schedules: [
                { id: 1, enabled: false, time: "06:30", weekday_mask: 62, portion_units: 1 },
                { id: 2, enabled: false, time: "12:00", weekday_mask: 62, portion_units: 1 },
                { id: 3, enabled: false, time: "18:00", weekday_mask: 62, portion_units: 1 },
                { id: 4, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 5, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 }
            ],
            rtc: {
                sync_threshold_ms: 3000
            }
        };

        this.saveConfig();

        return {
            success: true,
            message: 'Configuration reset to defaults'
        };
    }

    async syncTime(unixTime) {
        await this.delay(50);

        console.log(`[MOCK] Time sync called with timestamp: ${unixTime}`);
        console.log(`[MOCK] That's: ${new Date(unixTime * 1000).toISOString()}`);

        return {
            success: true,
            message: 'Time synchronized successfully (mock)'
        };
    }

    // Helper method to convert weekday_mask to array
    weekdayMaskToArray(mask) {
        const days = [];
        const dayNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
        
        for (let i = 0; i < 7; i++) {
            if (mask & (1 << i)) {
                days.push(i);
            }
        }
        return days;
    }

    // Helper method to convert array to weekday_mask
    arrayToWeekdayMask(days) {
        let mask = 0;
        days.forEach(day => {
            mask |= (1 << day);
        });
        return mask;
    }
}

// Global API instance
window.mockAPI = new MockAPI();

// Console helper for testing
console.log('Mock API loaded. Available methods:');
console.log('- mockAPI.getStatus()');
console.log('- mockAPI.getConfig()');
console.log('- mockAPI.saveConfig(config)');
console.log('- mockAPI.triggerFeed()');
console.log('- mockAPI.resetConfig()');
console.log('- mockAPI.syncTime(unixTime)');
