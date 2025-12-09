// ESP32-C3 Configuration Service - In-Memory Storage
// Simulates SPIFFS/LittleFS behavior

class ConfigService {
    constructor() {
        // Initialize with default config (like ESP32 would)
        this.resetToDefaults();
    }

    resetToDefaults() {
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
        console.log('[ConfigService] Configuration reset to defaults');
    }

    getConfig() {
        // Deep copy to prevent external modifications
        return JSON.parse(JSON.stringify(this.config));
    }

    updateConfig(updates) {
        try {
            // Validate updates
            if (updates.portion_unit_grams !== undefined) {
                const portion = parseInt(updates.portion_unit_grams);
                if (isNaN(portion) || portion < 1 || portion > 100) {
                    throw new Error('Invalid portion size. Must be between 1-100 grams.');
                }
                this.config.portion_unit_grams = portion;
            }

            if (updates.schedules) {
                // Validate schedules array
                if (!Array.isArray(updates.schedules)) {
                    throw new Error('Schedules must be an array');
                }
                
                // Update only the provided schedules
                updates.schedules.forEach((schedule, index) => {
                    if (index < this.config.schedules.length) {
                        this.config.schedules[index] = { ...this.config.schedules[index], ...schedule };
                    }
                });
            }

            console.log('[ConfigService] Configuration updated:', updates);
            return { success: true };
        } catch (error) {
            console.error('[ConfigService] Update failed:', error.message);
            return { success: false, error: error.message };
        }
    }

    // Helper method to get active schedules for current time/day
    getActiveSchedules() {
        const now = new Date();
        const currentDay = now.getDay(); // 0=Sunday, 1=Monday, etc.
        const currentTime = now.toTimeString().slice(0, 5); // HH:MM format
        
        return this.config.schedules.filter(schedule => {
            if (!schedule.enabled) return false;
            
            // Check if today is in the weekday mask
            const dayBit = 1 << currentDay;
            if (!(schedule.weekday_mask & dayBit)) return false;
            
            // For simplicity, we're not doing exact time matching here
            // In a real ESP32, this would trigger at the exact scheduled time
            return true;
        });
    }

    // Get human-readable schedule info
    getScheduleInfo() {
        const dayNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
        
        return this.config.schedules.map((schedule, index) => {
            const days = [];
            for (let i = 0; i < 7; i++) {
                if (schedule.weekday_mask & (1 << i)) {
                    days.push(dayNames[i]);
                }
            }
            
            return {
                id: schedule.id,
                enabled: schedule.enabled,
                time: schedule.time,
                days: days.join(', '),
                portion_grams: schedule.portion_units * this.config.portion_unit_grams
            };
        });
    }
}

// Singleton instance - simulates global config in ESP32
const configService = new ConfigService();

module.exports = configService;
