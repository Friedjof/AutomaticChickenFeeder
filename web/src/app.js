// Chicken Feeder Web Interface
// Main application logic

export class ChickenFeederApp {
    constructor() {
        this.apiBaseUrl = '/api'; // Relative API endpoint via reverse proxy
        this.elements = {};
        this.config = null;
        this.status = null;
        this.updateInterval = null;
        this.mockApi = window.mockAPI;
        this.useMock = !!this.mockApi && this.shouldUseMockFromQuery();
        this.mockLoadPromise = null;
        
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
        this.elements.servoPosition = document.getElementById('servoPosition');
        
        // Control elements
        this.elements.manualFeedBtn = document.getElementById('manualFeedBtn');
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

        // Schedule save button
        this.elements.saveScheduleBtn.addEventListener('click', () => this.saveScheduleOnly());

        // Timer events
        this.elements.timerRows.forEach((row, index) => {
            const expandBtn = row.querySelector('.expand-btn');
            const enabledCheckbox = row.querySelector('.timer-enabled');
            const timeInput = row.querySelector('.timer-time');
            const weekdayChecks = row.querySelectorAll('.weekday-check input[type="checkbox"]');
            const portionSlider = row.querySelector('.portion-slider');

            // Expand/collapse on expand button click
            if (expandBtn) {
                expandBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.toggleExpand(index);
                });
            }

            enabledCheckbox.addEventListener('change', () => this.updateTimerState(index));
            timeInput.addEventListener('change', () => this.updateTimerTime(index));

            weekdayChecks.forEach(checkbox => {
                checkbox.addEventListener('change', () => this.toggleWeekday(index, checkbox));
            });

            if (portionSlider) {
                portionSlider.addEventListener('input', () => this.updatePortionSlider(index));
            }
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
        if (this.useMock) {
            if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
            return await this.mockApi.getStatus();
        }
        return await this.apiRequest('/status');
    }

    async getConfig() {
        if (this.useMock) {
            if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
            return await this.mockApi.getConfig();
        }
        return await this.apiRequest('/config');
    }

    async saveConfig(config) {
        if (this.useMock) {
            if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
            return await this.mockApi.saveConfig(config);
        }
        return await this.apiRequest('/config', {
            method: 'POST',
            body: JSON.stringify(config)
        });
    }

    async resetConfig() {
        if (this.useMock) {
            if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
            return await this.mockApi.resetConfig();
        }
        return await this.apiRequest('/config/reset', {
            method: 'POST'
        });
    }

    async triggerFeed() {
        if (this.useMock) {
            if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
            return await this.mockApi.triggerFeed();
        }
        return await this.apiRequest('/feed', {
            method: 'POST'
        });
    }

    async loadInitialData() {
        try {
            if (this.useMock && !await this.ensureMockReady()) {
                throw new Error('Mock API script missing');
            }

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
            console.error('Error loading initial data:', error);
            if (!this.useMock && await this.enableMock('API unavailable')) {
                this.showToast('Using mock API (no device)', 'info');
                return this.loadInitialData();
            }
            this.showToast('Failed to connect to device', 'error');
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

        this.ensureSchedules(this.elements.timerRows.length);

        this.elements.timerRows.forEach((row, index) => {
            if (this.config.schedules[index]) {
                this.updateTimerRow(index, this.config.schedules[index]);
            }
        });
    }

    updateTimerRow(index, schedule) {
        const row = this.elements.timerRows[index];
        if (!row) return;

        const enabledCheckbox = row.querySelector('.timer-enabled');
        const timeInput = row.querySelector('.timer-time');
        const weekdayChecks = row.querySelectorAll('.weekday-check input[type="checkbox"]');
        const inlinePortion = row.querySelector('.portion-inline');

        // Update enabled state
        enabledCheckbox.checked = schedule.enabled;

        // Update time
        timeInput.value = schedule.time;

        // Update weekdays - set checkbox checked state
        weekdayChecks.forEach(checkbox => {
            const day = parseInt(checkbox.dataset.day);
            checkbox.checked = !!(schedule.weekday_mask & (1 << day));
        });

        // Update portion slider
        this.updatePortionDisplay(row, schedule.portion_units);
        if (inlinePortion) {
            inlinePortion.textContent = `${schedule.portion_units * this.getPortionUnitGrams()}g`;
        }

        // Update row state
        this.updateRowState(row, schedule.enabled);
    }

    updateRowState(row, enabled) {
        const timeInput = row.querySelector('.timer-time');
        const weekdayChecks = row.querySelectorAll('.weekday-check input[type="checkbox"]');
        const portionSlider = row.querySelector('.portion-slider');

        if (enabled) {
            row.classList.remove('disabled');
            row.classList.add('active');
            timeInput.disabled = false;
            weekdayChecks.forEach(checkbox => checkbox.disabled = false);
            if (portionSlider) portionSlider.disabled = false;
        } else {
            row.classList.add('disabled');
            row.classList.remove('active');
            timeInput.disabled = true;
            weekdayChecks.forEach(checkbox => checkbox.disabled = true);
            if (portionSlider) portionSlider.disabled = true;
        }
    }

    toggleExpand(index) {
        const row = this.elements.timerRows[index];
        row.classList.toggle('expanded');
    }

    updatePortionSlider(index) {
        if (!this.config) return;
        this.ensureSchedules(this.elements.timerRows.length);
        if (!this.config.schedules[index]) return;

        const row = this.elements.timerRows[index];
        const slider = row.querySelector('.portion-slider');
        const display = row.querySelector('.portion-display');
        const inlinePortion = row.querySelector('.portion-inline');

        const units = parseInt(slider.value);
        const grams = units * this.getPortionUnitGrams();

        // Update display
        if (display) display.textContent = `${grams}g`;
        if (inlinePortion) inlinePortion.textContent = `${grams}g`;

        // Update config
        this.config.schedules[index].portion_units = units;

        console.log(`Schedule ${index + 1}: Set portion to ${grams}g (${units} units)`);
    }

    updatePortionDisplay(row, portionUnits) {
        const slider = row.querySelector('.portion-slider');
        const display = row.querySelector('.portion-display');
        const inlinePortion = row.querySelector('.portion-inline');

        slider.value = portionUnits;
        const grams = portionUnits * this.getPortionUnitGrams();
        if (display) display.textContent = `${grams}g`;
        if (inlinePortion) inlinePortion.textContent = `${grams}g`;
    }

    updateTimerState(index) {
        if (!this.config) return;
        this.ensureSchedules(this.elements.timerRows.length);
        if (!this.config.schedules[index]) return;
        
        const row = this.elements.timerRows[index];
        const enabledCheckbox = row.querySelector('.timer-enabled');
        const enabled = enabledCheckbox.checked;
        
        this.config.schedules[index].enabled = enabled;
        this.updateRowState(row, enabled);
    }

    updateTimerTime(index) {
        if (!this.config) return;
        this.ensureSchedules(this.elements.timerRows.length);
        if (!this.config.schedules[index]) return;
        
        const row = this.elements.timerRows[index];
        const timeInput = row.querySelector('.timer-time');
        
        this.config.schedules[index].time = timeInput.value;
    }

    toggleWeekday(index, checkbox) {
        if (!this.config) return;
        this.ensureSchedules(this.elements.timerRows.length);
        if (!this.config.schedules[index]) return;
        if (checkbox.disabled) return;

        const schedule = this.config.schedules[index];
        const day = parseInt(checkbox.dataset.day);
        const dayBit = 1 << day;

        if (checkbox.checked) {
            // Add day
            schedule.weekday_mask = schedule.weekday_mask | dayBit;
        } else {
            // Remove day
            schedule.weekday_mask = schedule.weekday_mask & ~dayBit;
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

    getPortionUnitGrams() {
        return this.config?.portion_unit_grams || 12;
    }
    shouldUseMockFromQuery() {
        const params = new URLSearchParams(window.location.search);
        return params.has('mock') || params.get('api') === 'mock';
    }

    ensureSchedules(count) {
        if (!this.config) return;
        if (!Array.isArray(this.config.schedules)) {
            this.config.schedules = [];
        }
        while (this.config.schedules.length < count) {
            const id = this.config.schedules.length + 1;
            this.config.schedules.push({
                id,
                enabled: false,
                time: "00:00",
                weekday_mask: 0,
                portion_units: 1
            });
        }
    }

    async ensureMockReady() {
        if (this.mockApi) return true;
        await this.loadMockScript();
        this.mockApi = window.mockAPI || this.mockApi;
        return !!this.mockApi;
    }

    async loadMockScript() {
        if (this.mockLoadPromise) {
            return this.mockLoadPromise;
        }

        this.mockLoadPromise = new Promise(resolve => {
            const script = document.createElement('script');
            script.id = 'mock-api-loader';
            script.src = './mock/api.js';
            script.onload = resolve;
            script.onerror = resolve;
            document.head.appendChild(script);
        });

        await this.mockLoadPromise;
    }

    async enableMock(reason = '') {
        if (this.useMock) return true;
        const loaded = await this.ensureMockReady();
        if (loaded) {
            this.useMock = true;
            if (reason) console.warn('Using mock API:', reason);
            return true;
        }
        return false;
    }
}
