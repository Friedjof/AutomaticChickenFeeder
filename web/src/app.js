// Chicken Feeder Web Interface
// Main application logic

export class ChickenFeederApp {
    constructor() {
        this.apiBaseUrl = '/api'; // Relative API endpoint via reverse proxy
        this.elements = {};
        this.config = null;
        this.status = null;
        this.updateInterval = null;
        this.timeSyncInterval = null;
        this.mockApi = window.mockAPI;
        this.useMock = !!this.mockApi && this.shouldUseMockFromQuery();
        this.mockLoadPromise = null;
        this.feedHistory = [];
        this.feedHistoryInterval = null;
        this.feedHistoryRefreshMs = 15000;
        this.isFeedLogOpen = false;
        this.handleFeedLogEscape = this.handleFeedLogEscape.bind(this);
        this.guideCollapseMedia = window.matchMedia('(max-width: 900px)');
        this.handleGuideMediaChange = this.handleGuideMediaChange.bind(this);

        this.init();
    }

    init() {
        this.bindElements();
        this.bindEvents();
        this.setupGuideCollapse();
        this.loadInitialData();
        this.startStatusUpdates();
        this.startTimeSync();
        this.checkMaintenanceMode();
    }

    bindElements() {
        // Status elements
        this.elements.statusIndicator = document.getElementById('statusIndicator');
        this.elements.servoPosition = document.getElementById('servoPosition');
        this.elements.lastFeedTime = document.getElementById('lastFeedTime');
        this.elements.feedLogToggle = document.getElementById('feedLogToggle');
        this.elements.feedLogOverlay = document.getElementById('feedLogOverlay');
        this.elements.feedLogList = document.getElementById('feedLogList');
        this.elements.feedLogClose = document.getElementById('feedLogClose');
        this.elements.guidePanel = document.getElementById('guidePanel');
        this.elements.guideContent = document.getElementById('guideContent');
        this.elements.guideToggle = document.getElementById('guideToggle');
        this.elements.guideHintToggle = document.getElementById('guideHintToggle');

        // Control elements
        this.elements.manualFeedBtn = document.getElementById('manualFeedBtn');
        this.elements.saveScheduleBtn = document.getElementById('saveScheduleBtn');
        this.elements.deepSleepBtn = document.getElementById('deepSleepBtn');
        this.elements.exportConfigBtn = document.getElementById('exportConfigBtn');
        this.elements.importConfigInput = document.getElementById('importConfigInput');
        this.elements.portionUnitInput = document.getElementById('portionUnitInput');
        this.elements.uiVersion = document.getElementById('uiVersion');

        // OTA elements
        this.elements.otaFirmwareInput = document.getElementById('otaFirmwareInput');
        this.elements.otaUploadBtn = document.getElementById('otaUploadBtn');
        this.elements.otaStatus = document.getElementById('otaStatus');
        this.elements.otaStatusText = document.getElementById('otaStatusText');
        this.elements.otaProgressContainer = document.getElementById('otaProgressContainer');
        this.elements.otaProgressFill = document.getElementById('otaProgressFill');
        this.elements.otaProgressText = document.getElementById('otaProgressText');

        // Timer elements
        this.elements.timerRows = document.querySelectorAll('.timer-row');

        // Toast
        this.elements.toast = document.getElementById('toast');
        this.elements.toastMessage = document.getElementById('toastMessage');
    }

    bindEvents() {
        // Cleanup on page unload
        window.addEventListener('beforeunload', () => {
            this.destroy();
        });

        // Feed log popup
        if (this.elements.feedLogToggle && this.elements.feedLogOverlay) {
            this.elements.feedLogToggle.addEventListener('click', (e) => {
                e.stopPropagation();
                this.toggleFeedLog(true);
            });
            this.elements.feedLogClose?.addEventListener('click', () => this.toggleFeedLog(false));
            this.elements.feedLogOverlay.addEventListener('click', (e) => {
                if (e.target === this.elements.feedLogOverlay) {
                    this.toggleFeedLog(false);
                }
            });
            document.addEventListener('keydown', this.handleFeedLogEscape);
        }

        // Manual feed button
        this.elements.manualFeedBtn.addEventListener('click', () => this.triggerManualFeed());

        // Schedule save button
        this.elements.saveScheduleBtn.addEventListener('click', () => this.saveScheduleOnly());

        // Deep sleep button
        if (this.elements.deepSleepBtn) {
            this.bindDeepSleepGesture(this.elements.deepSleepBtn);
        }

        // Export config
        if (this.elements.exportConfigBtn) {
            this.elements.exportConfigBtn.addEventListener('click', () => this.exportConfig());
        }

        // Import config
        if (this.elements.importConfigInput) {
            this.elements.importConfigInput.addEventListener('change', (e) => this.importConfig(e));
        }

        // Portion unit change
        if (this.elements.portionUnitInput) {
            this.elements.portionUnitInput.addEventListener('change', () => this.updatePortionUnit());
            this.elements.portionUnitInput.addEventListener('input', () => this.updatePortionUnit());
        }

        // OTA firmware selection
        if (this.elements.otaFirmwareInput) {
            this.elements.otaFirmwareInput.addEventListener('change', (e) => this.onOtaFileSelected(e));
        }

        // OTA upload button
        if (this.elements.otaUploadBtn) {
            this.elements.otaUploadBtn.addEventListener('click', () => this.uploadOtaFirmware());
        }

        // Guide collapse (mobile)
        if (this.elements.guideToggle && this.elements.guidePanel) {
            this.elements.guideToggle.addEventListener('click', () => {
                const collapsed = this.elements.guidePanel.classList.contains('collapsed');
                this.setGuideCollapsed(!collapsed);
            });
        }
        if (this.elements.guideHintToggle && this.elements.guidePanel) {
            this.elements.guideHintToggle.addEventListener('click', () => {
                const collapsed = this.elements.guidePanel.classList.contains('collapsed');
                this.setGuideCollapsed(!collapsed);
            });
        }

        // UI/System version display (static fallback)
        if (this.elements.uiVersion) {
            const ver = typeof __APP_VERSION__ !== 'undefined' ? __APP_VERSION__ : 'v1.0.0';
            const link = document.createElement('a');
            link.href = `https://github.com/Friedjof/AutomaticChickenFeeder/releases/tag/${ver}`;
            link.target = '_blank';
            link.rel = 'noopener';
            link.textContent = ver;
            this.elements.uiVersion.innerHTML = 'Release ';
            this.elements.uiVersion.appendChild(link);
        }

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

    async getFeedHistory(limit = 10) {
        if (this.useMock) {
            if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
            return await this.mockApi.getFeedHistory(limit);
        }
        return await this.apiRequest(`/status/history?limit=${limit}`);
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

    async triggerDeepSleep() {
        try {
            this.elements.deepSleepBtn.disabled = true;
            this.elements.deepSleepBtn.textContent = 'Shutting down...';
            let response;
            if (this.useMock) {
                if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
                response = await this.mockApi.powerSleep();
            } else {
                response = await this.apiRequest('/power/sleep', { method: 'POST' });
            }
            if (response.success) {
                this.showToast('Device entering sleep', 'info');
            } else {
                this.showToast(response.error || 'Failed to sleep device', 'error');
            }
        } catch (error) {
            console.error('Error triggering sleep:', error);
            if (!this.useMock && await this.enableMock('sleep API unavailable')) {
                this.showToast('Using mock sleep (no device)', 'info');
                try {
                    const mockResp = await this.mockApi.powerSleep();
                    if (mockResp.success) {
                        this.showToast('Device entering sleep (mock)', 'info');
                    } else {
                        this.showToast(mockResp.error || 'Failed to sleep device', 'error');
                    }
                } catch (mockErr) {
                    console.error('Mock sleep failed:', mockErr);
                    this.showToast('Mock sleep failed', 'error');
                }
            } else {
                this.showToast('Network error', 'error');
            }
        } finally {
            setTimeout(() => {
                this.elements.deepSleepBtn.disabled = false;
                this.elements.deepSleepBtn.textContent = 'Shut down & sleep';
            }, 1500);
        }
    }

    async syncTime() {
        try {
            // Get current browser time as UTC Unix timestamp
            const unixTime = Math.floor(Date.now() / 1000);

            if (this.useMock) {
                if (!await this.ensureMockReady()) throw new Error('Mock API unavailable');
                return await this.mockApi.syncTime(unixTime);
            }

            const response = await this.apiRequest('/time', {
                method: 'POST',
                body: JSON.stringify({ unixTime })
            });

            if (!response.success) {
                console.error('[TIME] Failed to sync time:', response.error);
                return false;
            }

            console.log('[TIME] Time synchronized successfully');
            return true;
        } catch (error) {
            console.error('[TIME] Time sync error:', error);
            return false;
        }
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

            await this.loadFeedHistory();
            this.startFeedHistoryUpdates();
            
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

    async loadFeedHistory(limit = 10) {
        try {
            const response = await this.getFeedHistory(limit);
            if (response.success && response.data?.feeds) {
                this.feedHistory = response.data.feeds;
                this.updateFeedHistoryUI();
            }
        } catch (error) {
            console.error('Error loading feed history:', error);
        }
    }

    startFeedHistoryUpdates() {
        if (this.feedHistoryInterval) return;
        this.feedHistoryInterval = setInterval(() => {
            this.loadFeedHistory().catch(err => console.error('Feed history refresh failed:', err));
        }, this.feedHistoryRefreshMs);
    }

    stopFeedHistoryUpdates() {
        if (this.feedHistoryInterval) {
            clearInterval(this.feedHistoryInterval);
            this.feedHistoryInterval = null;
        }
    }

    startTimeSync() {
        // Sync immediately on start
        this.syncTime();

        // Then sync every 10 seconds
        this.timeSyncInterval = setInterval(() => {
            this.syncTime();
        }, 10000); // 10 seconds

        console.log('[TIME] Started periodic time sync (every 10s)');
    }

    stopTimeSync() {
        if (this.timeSyncInterval) {
            clearInterval(this.timeSyncInterval);
            this.timeSyncInterval = null;
            console.log('[TIME] Stopped periodic time sync');
        }
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
            this.elements.lastFeedTime.textContent = this.formatRelativeTime(this.status.lastFeedTime);
        } else {
            this.elements.lastFeedTime.textContent = 'Never';
        }
        
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

        if (this.elements.portionUnitInput && this.config.portion_unit_grams) {
            this.elements.portionUnitInput.value = this.config.portion_unit_grams;
        }
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
                setTimeout(() => this.loadFeedHistory(), 2200);
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
                schedules: this.config.schedules,
                portion_unit_grams: this.getPortionUnitGrams()
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
        this.stopFeedHistoryUpdates();
        document.removeEventListener('keydown', this.handleFeedLogEscape);
        if (this.guideCollapseMedia?.removeEventListener) {
            this.guideCollapseMedia.removeEventListener('change', this.handleGuideMediaChange);
        }
        this.stopTimeSync();
    }

    getPortionUnitGrams() {
        return this.config?.portion_unit_grams || 12;
    }
    shouldUseMockFromQuery() {
        const params = new URLSearchParams(window.location.search);
        return params.has('mock') || params.get('api') === 'mock';
    }

    setupGuideCollapse() {
        if (!this.elements.guidePanel || !this.elements.guideToggle || !this.elements.guideContent) return;
        if (this.guideCollapseMedia?.addEventListener) {
            this.guideCollapseMedia.addEventListener('change', this.handleGuideMediaChange);
        }
        this.handleGuideMediaChange(this.guideCollapseMedia);
    }

    handleGuideMediaChange(e) {
        const shouldCollapse = !!(e && e.matches);
        if (shouldCollapse) {
            this.setGuideCollapsed(true);
        } else {
            this.setGuideCollapsed(false, true);
        }
    }

    setGuideCollapsed(collapsed, force = false) {
        if (!this.elements.guidePanel || !this.elements.guideContent) return;
        const isMobile = !!this.guideCollapseMedia?.matches;
        if (!isMobile && !force) {
            collapsed = false;
        }
        this.elements.guidePanel.classList.toggle('collapsed', collapsed);
        if (this.elements.guideHintToggle) {
            this.elements.guideHintToggle.setAttribute('aria-expanded', collapsed ? 'false' : 'true');
            this.elements.guideHintToggle.querySelector('.guide-hint-text').textContent = collapsed ? 'Show guide' : 'Hide guide';
        }
    }

    toggleFeedLog(forceState) {
        if (!this.elements.feedLogOverlay || !this.elements.feedLogToggle) return;
        this.isFeedLogOpen = typeof forceState === 'boolean' ? forceState : !this.isFeedLogOpen;
        this.elements.feedLogOverlay.classList.toggle('open', this.isFeedLogOpen);
        this.elements.feedLogToggle.setAttribute('aria-expanded', this.isFeedLogOpen ? 'true' : 'false');
        this.elements.feedLogOverlay.setAttribute('aria-hidden', this.isFeedLogOpen ? 'false' : 'true');
    }

    handleFeedLogEscape(event) {
        if (event.key === 'Escape') {
            this.toggleFeedLog(false);
        }
    }

    updateFeedHistoryUI() {
        if (!this.elements.feedLogList) return;
        const list = this.elements.feedLogList;
        list.innerHTML = '';

        if (!this.feedHistory || this.feedHistory.length === 0) {
            const empty = document.createElement('li');
            empty.className = 'feed-log-empty';
            empty.textContent = 'No feed events yet';
            list.appendChild(empty);
            return;
        }

        this.feedHistory.forEach(feed => {
            const item = document.createElement('li');
            item.className = 'feed-log-item';

            const primary = document.createElement('div');
            primary.className = 'feed-log-primary';
            primary.textContent = this.formatFeedTimestamp(feed.timestamp);

            const meta = document.createElement('div');
            meta.className = 'feed-log-meta-line';
            const portion = feed.portion ? `${feed.portion}g` : '';
            const ago = feed.timeAgo || this.formatRelativeTime(feed.timestamp);
            meta.textContent = portion ? `${portion} • ${ago}` : ago;

            item.appendChild(primary);
            item.appendChild(meta);
            list.appendChild(item);
        });
    }

    formatFeedTimestamp(isoString) {
        const date = new Date(isoString);
        if (isNaN(date.getTime())) return isoString;
        const datePart = date.toLocaleDateString(undefined, { weekday: 'short', month: 'short', day: 'numeric' });
        const timePart = date.toLocaleTimeString(undefined, { hour: '2-digit', minute: '2-digit' });
        return `${datePart} · ${timePart}`;
    }

    formatRelativeTime(isoString) {
        const date = new Date(isoString);
        if (isNaN(date.getTime())) return isoString;

        const now = new Date();
        const diffMs = now - date;
        const diffMin = Math.floor(diffMs / 60000);
        const diffHr = Math.floor(diffMin / 60);
        const diffDay = Math.floor(diffHr / 24);

        if (diffMin < 1) return 'Just now';
        if (diffMin < 60) return `${diffMin}m ago`;
        if (diffHr < 24) return `${diffHr}h ago`;
        if (diffDay < 7) return `${diffDay}d ago`;

        return date.toLocaleString();
    }

    exportConfig() {
        if (!this.config || !this.config.schedules) {
            this.showToast('No config to export', 'error');
            return;
        }

        const blob = new Blob([JSON.stringify({ schedules: this.config.schedules, portion_unit_grams: this.getPortionUnitGrams() }, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        const ts = new Date();
        const stamp = `${ts.getFullYear()}-${String(ts.getMonth() + 1).padStart(2, '0')}-${String(ts.getDate()).padStart(2, '0')}_${String(ts.getHours()).padStart(2, '0')}-${String(ts.getMinutes()).padStart(2, '0')}`;
        a.download = `chicken-feeder-config_${stamp}.json`;
        a.click();
        URL.revokeObjectURL(url);
        this.showToast('Config exported', 'success');
    }

    importConfig(event) {
        const file = event.target.files?.[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (e) => {
            try {
                const parsed = JSON.parse(e.target.result);
                if (!parsed.schedules || !Array.isArray(parsed.schedules)) {
                    throw new Error('Invalid config format');
                }
                // Trim/extend to available rows
                this.ensureSchedules(this.elements.timerRows.length);
                parsed.schedules.slice(0, this.elements.timerRows.length).forEach((sched, idx) => {
                    this.config.schedules[idx] = {
                        ...this.config.schedules[idx],
                        ...sched
                    };
                });
                if (parsed.portion_unit_grams) {
                    this.config.portion_unit_grams = parsed.portion_unit_grams;
                }
                this.updateConfigurationUI();
                this.showToast('Config imported (not saved yet)', 'info');
            } catch (err) {
                console.error('Import failed:', err);
                this.showToast('Invalid JSON config', 'error');
            } finally {
                event.target.value = '';
            }
        };
        reader.readAsText(file);
    }

    updatePortionUnit() {
        if (!this.config) return;
        const val = parseInt(this.elements.portionUnitInput.value, 10);
        const grams = Math.min(100, Math.max(1, isNaN(val) ? this.getPortionUnitGrams() : val));
        this.config.portion_unit_grams = grams;
        this.elements.portionUnitInput.value = grams;

        // Refresh inline displays
        this.elements.timerRows.forEach((row, index) => {
            if (this.config.schedules[index]) {
                this.updatePortionDisplay(row, this.config.schedules[index].portion_units);
            }
        });
    }

    bindDeepSleepGesture(button) {
        let holdTimer = null;
        const holdDelay = 600; // ms

        const trigger = () => this.triggerDeepSleep();

        button.addEventListener('click', (e) => {
            // If a long press already triggered, ignore the click
            if (button.dataset.triggered === 'true') {
                button.dataset.triggered = '';
                e.preventDefault();
                return;
            }
            trigger();
        });

        const startHold = () => {
            holdTimer = setTimeout(() => {
                button.dataset.triggered = 'true';
                trigger();
            }, holdDelay);
        };

        const clearHold = () => {
            if (holdTimer) {
                clearTimeout(holdTimer);
                holdTimer = null;
            }
        };

        button.addEventListener('mousedown', startHold);
        button.addEventListener('touchstart', startHold);
        button.addEventListener('mouseup', clearHold);
        button.addEventListener('mouseleave', clearHold);
        button.addEventListener('touchend', clearHold);
        button.addEventListener('touchcancel', clearHold);
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

    // OTA Firmware Update Methods
    onOtaFileSelected(event) {
        const file = event.target.files?.[0];
        if (!file) {
            this.elements.otaUploadBtn.disabled = true;
            return;
        }

        // Check if file is a .bin file
        if (!file.name.endsWith('.bin')) {
            this.showToast('Please select a .bin firmware file', 'error');
            event.target.value = '';
            this.elements.otaUploadBtn.disabled = true;
            return;
        }

        // Enable upload button
        this.elements.otaUploadBtn.disabled = false;
        this.setOtaStatus('offline', `Ready: ${file.name}`);
    }

    async uploadOtaFirmware() {
        const file = this.elements.otaFirmwareInput.files?.[0];
        if (!file) {
            this.showToast('No file selected', 'error');
            return;
        }

        try {
            // Disable controls during upload
            this.elements.otaUploadBtn.disabled = true;
            this.elements.otaFirmwareInput.disabled = true;
            this.setOtaStatus('uploading', 'Uploading firmware...');
            this.elements.otaProgressContainer.style.display = 'block';

            // Create form data
            const formData = new FormData();
            formData.append('firmware', file);

            // Upload with progress tracking
            const xhr = new XMLHttpRequest();

            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const percentComplete = Math.round((e.loaded / e.total) * 100);
                    this.updateOtaProgress(percentComplete);
                }
            });

            xhr.addEventListener('load', () => {
                if (xhr.status === 200) {
                    try {
                        const response = JSON.parse(xhr.responseText);
                        if (response.success) {
                            this.setOtaStatus('online', 'Upload successful! Device rebooting...');
                            this.showToast('Firmware uploaded successfully! Device will reboot.', 'success');
                            this.updateOtaProgress(100);

                            // Reset after 5 seconds
                            setTimeout(() => {
                                this.resetOtaUI();
                            }, 5000);
                        } else {
                            throw new Error(response.error || 'Upload failed');
                        }
                    } catch (error) {
                        this.handleOtaError(error.message || 'Invalid response from device');
                    }
                } else {
                    this.handleOtaError(`Upload failed (HTTP ${xhr.status})`);
                }
            });

            xhr.addEventListener('error', () => {
                this.handleOtaError('Network error during upload');
            });

            xhr.addEventListener('abort', () => {
                this.handleOtaError('Upload cancelled');
            });

            xhr.open('POST', `${this.apiBaseUrl}/ota/update`);
            xhr.send(formData);

        } catch (error) {
            this.handleOtaError(error.message);
        }
    }

    setOtaStatus(status, text) {
        const statusDot = this.elements.otaStatus.querySelector('.status-dot');
        statusDot.className = `status-dot ${status}`;
        this.elements.otaStatusText.textContent = text;
    }

    updateOtaProgress(percent) {
        this.elements.otaProgressFill.style.width = `${percent}%`;
        this.elements.otaProgressText.textContent = `${percent}%`;
    }

    handleOtaError(message) {
        this.setOtaStatus('offline', `Error: ${message}`);
        this.showToast(message, 'error');
        this.resetOtaUI();
    }

    resetOtaUI() {
        this.elements.otaUploadBtn.disabled = false;
        this.elements.otaFirmwareInput.disabled = false;
        this.elements.otaFirmwareInput.value = '';
        this.elements.otaUploadBtn.disabled = true;
        this.elements.otaProgressContainer.style.display = 'none';
        this.updateOtaProgress(0);
        this.setOtaStatus('offline', 'Ready for update');
    }

    async checkMaintenanceMode() {
        try {
            const response = await this.apiRequest('/ota/status');
            if (response && response.success) {
                this.elements.otaFirmwareInput.disabled = false;
                this.setOtaStatus('online', 'Ready for firmware update');
            } else {
                this.elements.otaFirmwareInput.disabled = true;
                this.setOtaStatus('offline', 'OTA not available');
            }
        } catch (error) {
            // If API not available, still enable OTA (backward compatibility)
            this.elements.otaFirmwareInput.disabled = false;
            this.setOtaStatus('online', 'Ready for firmware update');
        }
    }
}
