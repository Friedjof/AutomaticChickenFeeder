// ESP32-C3 Servo Simulation Service
// Simulates realistic servo motor behavior

class ServoService {
    constructor() {
        this.position = 'closed';  // closed, opening, open, closing
        this.isFeeding = false;
        this.feedHistory = [];
        this.totalFedToday = 0;
        this.lastResetDate = new Date().toDateString();
        
        // Simulate some initial feeds for demo
        this.simulateInitialFeeds();
        
        console.log('[ServoService] Servo simulation initialized');
    }

    simulateInitialFeeds() {
        // Reset daily counter if it's a new day
        const today = new Date().toDateString();
        if (this.lastResetDate !== today) {
            this.totalFedToday = 0;
            this.lastResetDate = today;
        }
        
        // Add some demo feeds from earlier today
        const now = new Date();
        const twoHoursAgo = new Date(now.getTime() - 2 * 60 * 60 * 1000);
        const fourHoursAgo = new Date(now.getTime() - 4 * 60 * 60 * 1000);
        
        this.feedHistory = [
            { timestamp: fourHoursAgo, portion: 12 },
            { timestamp: twoHoursAgo, portion: 12 }
        ];
        
        this.totalFedToday = 24; // 2 feeds of 12g each
    }

    getStatus() {
        // Reset daily counter if it's a new day
        const today = new Date().toDateString();
        if (this.lastResetDate !== today) {
            this.totalFedToday = 0;
            this.lastResetDate = today;
            this.feedHistory = this.feedHistory.filter(feed => 
                feed.timestamp.toDateString() === today
            );
        }

        const lastFeed = this.feedHistory.length > 0 
            ? this.feedHistory[this.feedHistory.length - 1] 
            : null;

        return {
            isOnline: true,
            servoPosition: this.position === 'closed' ? 'Closed' : 
                         this.position === 'opening' ? 'Opening' :
                         this.position === 'open' ? 'Open' : 'Closing',
            isFeeding: this.isFeeding,
            lastFeedTime: lastFeed ? lastFeed.timestamp.toISOString() : null,
            totalFedToday: this.totalFedToday
        };
    }

    async triggerFeed(portionGrams = 12) {
        if (this.isFeeding) {
            return { 
                success: false, 
                error: 'Feeder is already active' 
            };
        }

        try {
            console.log(`[ServoService] Starting feed cycle: ${portionGrams}g`);
            
            this.isFeeding = true;
            
            // Simulate realistic servo timing
            await this.simulateServoMovement(portionGrams);
            
            // Record the feed
            this.recordFeed(portionGrams);
            
            this.isFeeding = false;
            
            console.log(`[ServoService] Feed cycle completed: ${portionGrams}g`);
            
            return { 
                success: true, 
                message: 'Feed cycle completed',
                portion: portionGrams
            };
            
        } catch (error) {
            this.isFeeding = false;
            this.position = 'closed';
            console.error('[ServoService] Feed cycle failed:', error.message);
            
            return { 
                success: false, 
                error: 'Feed cycle failed: ' + error.message 
            };
        }
    }

    async simulateServoMovement(portionGrams) {
        // Phase 1: Opening (500ms)
        this.position = 'opening';
        await this.delay(500);
        
        // Phase 2: Open and dispensing (duration based on portion size)
        this.position = 'open';
        const dispensingTime = Math.max(1000, portionGrams * 50); // 50ms per gram, min 1s
        await this.delay(dispensingTime);
        
        // Phase 3: Closing (300ms)
        this.position = 'closing';
        await this.delay(300);
        
        // Phase 4: Closed
        this.position = 'closed';
    }

    recordFeed(portionGrams) {
        const feedRecord = {
            timestamp: new Date(),
            portion: portionGrams
        };
        
        this.feedHistory.push(feedRecord);
        this.totalFedToday += portionGrams;
        
        // Keep only last 50 feeds to prevent memory buildup
        if (this.feedHistory.length > 50) {
            this.feedHistory = this.feedHistory.slice(-50);
        }
        
        console.log(`[ServoService] Feed recorded: ${portionGrams}g (Total today: ${this.totalFedToday}g)`);
    }

    getFeedHistory(limit = 10) {
        return this.feedHistory
            .slice(-limit)
            .reverse()
            .map(feed => ({
                timestamp: feed.timestamp.toISOString(),
                portion: feed.portion,
                timeAgo: this.getTimeAgo(feed.timestamp)
            }));
    }

    getTimeAgo(timestamp) {
        const now = new Date();
        const diffMs = now - timestamp;
        const diffMins = Math.floor(diffMs / (1000 * 60));
        const diffHours = Math.floor(diffMins / 60);
        const diffDays = Math.floor(diffHours / 24);

        if (diffDays > 0) return `${diffDays}d ago`;
        if (diffHours > 0) return `${diffHours}h ago`;
        if (diffMins > 0) return `${diffMins}m ago`;
        return 'Just now';
    }

    // Utility method for async delays
    delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    // Simulate hardware error (for testing)
    simulateError() {
        this.isFeeding = false;
        this.position = 'closed';
        throw new Error('Servo motor stuck or disconnected');
    }

    // Reset daily statistics (called at midnight in real ESP32)
    resetDailyStats() {
        this.totalFedToday = 0;
        this.lastResetDate = new Date().toDateString();
        console.log('[ServoService] Daily statistics reset');
    }
}

// Singleton instance - simulates hardware state in ESP32
const servoService = new ServoService();

module.exports = servoService;
