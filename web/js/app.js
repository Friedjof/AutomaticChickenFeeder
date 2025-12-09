// Chicken Feeder Web Interface
// Main application logic

class ChickenFeederApp {
    constructor() {
        this.apiBaseUrl = '/api'; // Relative API endpoint via reverse proxy
        this.elements = {};
        this.config = null;
        this.status = null;
        this.updateInterval = null;
        
        this.init();
    }

    init() {
        this.bindElements();
        this.bindEvents();
        this.loadInitialData();
        this.startStatusUpdates();
    }

    bindElements() {
        // Status elements
        this.elements.statusIndicator = document.getElementById('statusIndicator');
        this.elements.lastFeedTime = document.getElementById('lastFeedTime');
        this.elements.servoPosition = document.getElementById('servoPosition');
        this.elements.portionSize = document.getElementById('portionSize');
        this.elements.totalFedToday = document.getElementById('totalFedToday');
        
        // Control elements
        this.elements.manualFeedBtn = document.getElementById('manualFeedBtn');
        this.elements.portionSlider = document.getElementById('portionSlider');
        this.elements.saveConfigBtn = document.getElementById('saveConfigBtn');
        this.elements.resetConfigBtn = document.getElementById('resetConfigBtn');
        this.elements.saveScheduleBtn = document.getElementById('saveScheduleBtn');
        
        // Timer elements
        this.elements.timerRows = document.querySelectorAll('.timer-row');
        
        // Toast
        this.elements.toast = document.getElementById('toast');
        this.elements.toastMessage = document.getElementById('toastMessage');
    }

    bindEvents() {
        // Manual feed button
        this.elements.manualFeedBtn.addEventListener('click', () => this.triggerManualFeed());
        
        // Configuration buttons
        this.elements.saveConfigBtn.addEventListener('click', () => this.saveConfiguration());
        this.elements.resetConfigBtn.addEventListener('click', () => this.resetConfiguration());
        this.elements.saveScheduleBtn.addEventListener('click', () => this.saveScheduleOnly());
        
        // Portion size slider
        this.elements.portionSlider.addEventListener('input', () => this.updatePortionPreview());
        
        // Timer events
        this.elements.timerRows.forEach((row, index) => {
            const enabledCheckbox = row.querySelector('.timer-enabled');
            const timeInput = row.querySelector('.timer-time');
            const weekdayBtns = row.querySelectorAll('.weekday-mini');
            
            enabledCheckbox.addEventListener('change', () => this.updateTimerState(index));
            timeInput.addEventListener('change', () => this.updateTimerTime(index));
            
            weekdayBtns.forEach(btn => {
                btn.addEventListener('click', () => this.toggleWeekday(index, btn));
            });
        });
    }

    // Real API Methods
    async apiRequest(endpoint, options = {}) {
        try {
            const response = await fetch(`${this.apiBaseUrl}${endpoint}`, {
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                },
                ...options
            });
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            return await response.json();
        } catch (error) {
            console.error('API request failed:', error);
            throw error;
        }
    }

    async getStatus() {
        return await this.apiRequest('/status');
    }

    async getConfig() {
        return await this.apiRequest('/config');
    }

    async saveConfig(config) {
        return await this.apiRequest('/config', {
            method: 'POST',
            body: JSON.stringify(config)
        });
    }

    async resetConfig() {
        return await this.apiRequest('/config/reset', {
            method: 'POST'
        });
    }

    async triggerFeed() {
        return await this.apiRequest('/feed', {
            method: 'POST'
        });
    }

    async loadInitialData() {
        try {
            // Load configuration
            const configResponse = await this.getConfig();
            if (configResponse.success) {
                this.config = configResponse.data;
                this.updateConfigurationUI();
            }
            
            // Load status
            const statusResponse = await this.getStatus();
            if (statusResponse.success) {
                this.status = statusResponse.data;
                this.updateStatusUI();
            }
            
        } catch (error) {
            this.showToast('Failed to connect to device', 'error');
            console.error('Error loading initial data:', error);
            this.updateOfflineStatus();
        }
    }

    startStatusUpdates() {
        // Update status every 2 seconds
        this.updateInterval = setInterval(() => {
            this.updateStatus();
        }, 2000);
    }

    async updateStatus() {
        try {
            const response = await this.getStatus();
            if (response.success) {
                this.status = response.data;
                this.updateStatusUI();
            }
        } catch (error) {
            console.error('Error updating status:', error);
            this.updateOfflineStatus();
        }
    }

    updateStatusUI() {
        if (!this.status) return;
        
        // Update connection status
        const statusDot = this.elements.statusIndicator.querySelector('.status-dot');
        const statusText = this.elements.statusIndicator.querySelector('span:last-child');
        
        if (this.status.isOnline) {
            statusDot.classList.remove('offline');
            statusDot.classList.add('online');
            statusText.textContent = 'Online';
        } else {
            statusDot.classList.remove('online');
            statusDot.classList.add('offline');
            statusText.textContent = 'Offline';
        }
        
        // Update servo position
        this.elements.servoPosition.textContent = this.status.servoPosition;
        
        // Update last feed time
        if (this.status.lastFeedTime) {
            const feedTime = new Date(this.status.lastFeedTime);
            const now = new Date();
            const diffMinutes = Math.floor((now - feedTime) / (1000 * 60));
            
            if (diffMinutes < 60) {
                this.elements.lastFeedTime.textContent = `${diffMinutes}m ago`;
            } else {
                const diffHours = Math.floor(diffMinutes / 60);
                this.elements.lastFeedTime.textContent = `${diffHours}h ago`;
            }
        } else {
            this.elements.lastFeedTime.textContent = 'Never';
        }
        
        // Update total fed today
        this.elements.totalFedToday.textContent = `${this.status.totalFedToday}g`;
        
        // Update feed button state
        if (this.status.isFeeding) {
            this.elements.manualFeedBtn.classList.add('feeding');
            this.elements.manualFeedBtn.disabled = true;
            this.elements.manualFeedBtn.querySelector('span:last-child').textContent = 'Feeding...';
        } else {
            this.elements.manualFeedBtn.classList.remove('feeding');
            this.elements.manualFeedBtn.disabled = false;
            this.elements.manualFeedBtn.querySelector('span:last-child').textContent = 'Feed Now';
        }
    }

    updateOfflineStatus() {
        const statusDot = this.elements.statusIndicator.querySelector('.status-dot');
        const statusText = this.elements.statusIndicator.querySelector('span:last-child');
        
        statusDot.classList.remove('online');
        statusDot.classList.add('offline');
        statusText.textContent = 'Offline';
    }

    updateConfigurationUI() {
        if (!this.config) return;
        
        // Update portion size slider
        this.elements.portionSlider.value = this.config.portion_unit_grams;
        this.updatePortionPreview();
        
        // Update timer rows (only first 3)
        this.elements.timerRows.forEach((row, index) => {
            if (index < 3 && this.config.schedules[index]) {
                this.updateTimerRow(index, this.config.schedules[index]);
            }
        });
    }

    updateTimerRow(index, schedule) {
        const row = this.elements.timerRows[index];
        if (!row) return;
        
        const enabledCheckbox = row.querySelector('.timer-enabled');
        const timeInput = row.querySelector('.timer-time');
        const weekdayBtns = row.querySelectorAll('.weekday-mini');
        
        // Update enabled state
        enabledCheckbox.checked = schedule.enabled;
        
        // Update time
        timeInput.value = schedule.time;
        
        // Update weekdays
        weekdayBtns.forEach(btn => {
            const day = parseInt(btn.dataset.day);
            if (schedule.weekday_mask & (1 << day)) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });
        
        // Update row state
        this.updateRowState(row, schedule.enabled);
    }

    updateRowState(row, enabled) {
        const timeInput = row.querySelector('.timer-time');
        const weekdayBtns = row.querySelectorAll('.weekday-mini');
        
        if (enabled) {
            row.classList.remove('disabled');
            timeInput.disabled = false;
            weekdayBtns.forEach(btn => btn.disabled = false);
        } else {
            row.classList.add('disabled');
            timeInput.disabled = true;
            weekdayBtns.forEach(btn => btn.disabled = true);
        }
    }

    updatePortionPreview() {
        const portionSize = this.elements.portionSlider.value;
        this.elements.portionSize.textContent = `${portionSize}g`;
    }

    updateTimerState(index) {
        if (!this.config || !this.config.schedules[index]) return;
        
        const row = this.elements.timerRows[index];
        const enabledCheckbox = row.querySelector('.timer-enabled');
        const enabled = enabledCheckbox.checked;
        
        this.config.schedules[index].enabled = enabled;
        this.updateRowState(row, enabled);
    }

    updateTimerTime(index) {
        if (!this.config || !this.config.schedules[index]) return;
        
        const row = this.elements.timerRows[index];
        const timeInput = row.querySelector('.timer-time');
        
        this.config.schedules[index].time = timeInput.value;
    }

    toggleWeekday(index, button) {
        if (!this.config || !this.config.schedules[index]) return;
        if (button.disabled) return;
        
        const schedule = this.config.schedules[index];
        const day = parseInt(button.dataset.day);
        const dayBit = 1 << day;
        
        if (schedule.weekday_mask & dayBit) {
            // Remove day
            schedule.weekday_mask = schedule.weekday_mask & ~dayBit;
            button.classList.remove('active');
        } else {
            // Add day
            schedule.weekday_mask = schedule.weekday_mask | dayBit;
            button.classList.add('active');
        }
    }

    async triggerManualFeed() {
        try {
            this.elements.manualFeedBtn.disabled = true;
            
            const response = await this.triggerFeed();
            
            if (response.success) {
                this.showToast('Feed cycle started!', 'success');
            } else {
                this.showToast(response.error || 'Failed to start feed cycle', 'error');
            }
            
        } catch (error) {
            this.showToast('Network error occurred', 'error');
            console.error('Error triggering feed:', error);
        } finally {
            // Button will be re-enabled by status update
            setTimeout(() => {
                this.elements.manualFeedBtn.disabled = false;
            }, 1000);
        }
    }

    async saveConfiguration() {
        try {
            const newConfig = {
                portion_unit_grams: parseInt(this.elements.portionSlider.value),
                schedules: this.config.schedules
            };
            
            const response = await this.saveConfig(newConfig);
            
            if (response.success) {
                this.showToast('Configuration saved!', 'success');
                this.config = { ...this.config, ...newConfig };
            } else {
                this.showToast(response.error || 'Failed to save configuration', 'error');
            }
            
        } catch (error) {
            this.showToast('Network error occurred', 'error');
            console.error('Error saving configuration:', error);
        }
    }

    async saveScheduleOnly() {
        try {
            const scheduleConfig = {
                schedules: this.config.schedules
            };
            
            const response = await this.saveConfig(scheduleConfig);
            
            if (response.success) {
                this.showToast('Schedule saved!', 'success');
                this.config = { ...this.config, ...scheduleConfig };
            } else {
                this.showToast(response.error || 'Failed to save schedule', 'error');
            }
            
        } catch (error) {
            this.showToast('Network error occurred', 'error');
            console.error('Error saving schedule:', error);
        }
    }

    async resetConfiguration() {
        if (!confirm('Reset all settings to default?')) {
            return;
        }
        
        try {
            const response = await this.resetConfig();
            
            if (response.success) {
                this.showToast('Settings reset to defaults', 'success');
                // Reload configuration
                await this.loadInitialData();
            } else {
                this.showToast(response.error || 'Failed to reset configuration', 'error');
            }
            
        } catch (error) {
            this.showToast('Network error occurred', 'error');
            console.error('Error resetting configuration:', error);
        }
    }

    showToast(message, type = 'info') {
        this.elements.toastMessage.textContent = message;
        this.elements.toast.className = `toast ${type}`;
        this.elements.toast.classList.add('show');
        
        setTimeout(() => {
            this.elements.toast.classList.remove('show');
        }, 3000);
    }

    // Cleanup on page unload
    destroy() {
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
        }
    }
}

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.app = new ChickenFeederApp();
    
    // Cleanup on page unload
    window.addEventListener('beforeunload', () => {
        if (window.app) {
            window.app.destroy();
        }
    });
});
