// Mock API for Chicken Feeder Development
// Simulates ESP32 API responses

class MockAPI {
    constructor() {
        // Load initial config from localStorage or use defaults
        this.config = this.loadConfig();
        this.deviceTimeMs = Date.now();
        this.deviceTimeStartedAtMs = Date.now();
        this.status = {
            isOnline: true,
            servoPosition: 'Closed',
            lastFeedTime: null,
            totalFedToday: 0,
            isFeeding: false
        };

        this.feedHistory = this.buildInitialFeedHistory();
        this.status.lastFeedTime = this.feedHistory[0]?.timestamp || null;
        this.status.totalFedToday = this.calculateTotalFedToday();
        
        // Simulate some previous feeds for demo
    }

    loadConfig() {
        const saved = localStorage.getItem('chicken-feeder-config');
        if (saved) {
            return this.normalizeConfig(JSON.parse(saved));
        }
        
        // Default config based on data-template/config.json
        return {
            version: 1,
            portion_unit_grams: 12,
            manual_portion_units: 1,
            schedules: [
                { id: 1, enabled: true, time: "06:30", weekday_mask: 62, portion_units: 1 },
                { id: 2, enabled: true, time: "12:00", weekday_mask: 62, portion_units: 1 },
                { id: 3, enabled: true, time: "18:00", weekday_mask: 62, portion_units: 1 },
                { id: 4, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 5, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 6, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 }
            ],
            rtc: {
                sync_threshold_ms: 3000
            }
        };
    }

    normalizeConfig(config) {
        const normalized = {
            version: 1,
            portion_unit_grams: 12,
            manual_portion_units: 1,
            schedules: [
                { id: 1, enabled: true, time: "06:30", weekday_mask: 62, portion_units: 1 },
                { id: 2, enabled: true, time: "12:00", weekday_mask: 62, portion_units: 1 },
                { id: 3, enabled: true, time: "18:00", weekday_mask: 62, portion_units: 1 },
                { id: 4, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 5, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 6, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 }
            ],
            rtc: {
                sync_threshold_ms: 3000
            },
            ...config
        };

        if (!Number.isInteger(normalized.manual_portion_units) || normalized.manual_portion_units < 1 || normalized.manual_portion_units > 10) {
            normalized.manual_portion_units = 1;
        }

        if (Array.isArray(normalized.schedules)) {
            normalized.schedules = normalized.schedules.map((schedule, index) => ({
                id: schedule?.id || index + 1,
                enabled: !!schedule?.enabled,
                time: schedule?.time || "00:00",
                weekday_mask: Number.isInteger(schedule?.weekday_mask) ? schedule.weekday_mask : 0,
                portion_units: Number.isInteger(schedule?.portion_units) && schedule.portion_units >= 1 && schedule.portion_units <= 10
                    ? schedule.portion_units
                    : 1
            }));
        }

        return normalized;
    }

    persistConfig() {
        localStorage.setItem('chicken-feeder-config', JSON.stringify(this.config));
    }

    getCurrentDeviceDate() {
        const elapsed = Date.now() - this.deviceTimeStartedAtMs;
        return new Date(this.deviceTimeMs + elapsed);
    }

    getCurrentTimeZoneLabel(date = this.getCurrentDeviceDate()) {
        const offsetMinutes = -date.getTimezoneOffset();
        return offsetMinutes >= 120 ? 'CEST' : 'CET';
    }

    // Simulate network delay
    async delay(ms = 200 + Math.random() * 300) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    // API Endpoints
    async getStatus() {
        await this.delay();
        this.status.lastFeedTime = this.feedHistory[0]?.timestamp || null;
        this.status.totalFedToday = this.calculateTotalFedToday();
        return {
            success: true,
            data: { ...this.status }
        };
    }

    async powerSleep() {
        await this.delay(150);
        return {
            success: true,
            message: 'Device entering deep sleep (mock)'
        };
    }

    async getConfig() {
        await this.delay();
        return {
            success: true,
            data: { ...this.config }
        };
    }

    async getTime() {
        await this.delay(50);

        const now = this.getCurrentDeviceDate();

        return {
            success: true,
            data: {
                year: now.getFullYear(),
                month: now.getMonth() + 1,
                day: now.getDate(),
                hour: now.getHours(),
                minute: now.getMinutes(),
                second: now.getSeconds(),
                timezone: this.getCurrentTimeZoneLabel(now),
                utc_offset_seconds: -now.getTimezoneOffset() * 60
            }
        };
    }

    async saveConfig(newConfig) {
        await this.delay();

        // Validate schedules if provided
        if (newConfig.schedules) {
            for (const schedule of newConfig.schedules) {
                if (schedule.portion_units < 1 || schedule.portion_units > 10) {
                    return {
                        success: false,
                        error: 'Invalid portion size. Must be between 1-10 units.'
                    };
                }
            }
        }

        if (Object.prototype.hasOwnProperty.call(newConfig, 'manual_portion_units')) {
            if (!Number.isInteger(newConfig.manual_portion_units) || newConfig.manual_portion_units < 1 || newConfig.manual_portion_units > 10) {
                return {
                    success: false,
                    error: 'Invalid manual feed amount. Must be between 1-10 units.'
                };
            }
        }

        this.config = this.normalizeConfig({ ...this.config, ...newConfig });
        this.persistConfig();

        return {
            success: true,
            message: 'Configuration saved successfully'
        };
    }

    async getFeedHistory(limit = 10) {
        await this.delay();
        const feeds = this.feedHistory.slice(0, limit).map(feed => ({
            ...feed,
            timeAgo: this.formatTimeAgo(feed.timestamp)
        }));
        return {
            success: true,
            data: {
                feeds,
                totalFeeds: this.feedHistory.length
            }
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
            this.recordFeed(this.config.manual_portion_units * this.config.portion_unit_grams);
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
            manual_portion_units: 1,
            schedules: [
                { id: 1, enabled: false, time: "06:30", weekday_mask: 62, portion_units: 1 },
                { id: 2, enabled: false, time: "12:00", weekday_mask: 62, portion_units: 1 },
                { id: 3, enabled: false, time: "18:00", weekday_mask: 62, portion_units: 1 },
                { id: 4, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 5, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 },
                { id: 6, enabled: false, time: "00:00", weekday_mask: 0, portion_units: 1 }
            ],
            rtc: {
                sync_threshold_ms: 3000
            }
        };

        this.persistConfig();

        return {
            success: true,
            message: 'Configuration reset to defaults'
        };
    }

    async syncTime(unixTime) {
        await this.delay(50);

        this.deviceTimeMs = unixTime * 1000;
        this.deviceTimeStartedAtMs = Date.now();

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

    buildInitialFeedHistory() {
        const now = new Date();
        const entries = [];
        for (let i = 0; i < 10; i++) {
            const ts = new Date(now.getTime() - (i + 1) * 90 * 60 * 1000);
            entries.push({
                timestamp: ts.toISOString(),
                portion: this.config?.portion_unit_grams || 12
            });
        }
        return entries;
    }

    calculateTotalFedToday() {
        const today = new Date().toDateString();
        return this.feedHistory.reduce((sum, entry) => {
            const entryDay = new Date(entry.timestamp).toDateString();
            return entryDay === today ? sum + (entry.portion || 0) : sum;
        }, 0);
    }

    recordFeed(portion) {
        const timestamp = new Date().toISOString();
        this.feedHistory.unshift({ timestamp, portion });
        if (this.feedHistory.length > 50) {
            this.feedHistory = this.feedHistory.slice(0, 50);
        }
        this.status.lastFeedTime = timestamp;
        this.status.totalFedToday = this.calculateTotalFedToday();
    }

    formatTimeAgo(timestamp) {
        const date = new Date(timestamp);
        const now = new Date();
        const diffMs = now - date;
        const diffMins = Math.floor(diffMs / 60000);
        const diffHours = Math.floor(diffMins / 60);
        const diffDays = Math.floor(diffHours / 24);

        if (diffDays > 0) return `${diffDays}d ago`;
        if (diffHours > 0) return `${diffHours}h ago`;
        if (diffMins > 0) return `${diffMins}m ago`;
        return 'Just now';
    }
}

// Global API instance
window.mockAPI = new MockAPI();

// Console helper for testing
console.log('Mock API loaded. Available methods:');
console.log('- mockAPI.getStatus()');
console.log('- mockAPI.getConfig()');
console.log('- mockAPI.getTime()');
console.log('- mockAPI.saveConfig(config)');
console.log('- mockAPI.triggerFeed()');
console.log('- mockAPI.resetConfig()');
console.log('- mockAPI.syncTime(unixTime)');
console.log('- mockAPI.getFeedHistory(limit)');
