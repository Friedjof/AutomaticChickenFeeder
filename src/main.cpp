#include <Arduino.h>
#include <esp_sleep.h>

#include "FeedingService.hpp"
#include "ButtonService.hpp"
#include "ClockService.hpp"
#include "ConfigService.hpp"
#include "WebService.hpp"
#include "SchedulingService.hpp"
#include "VibrationService.hpp"
#include "PinConfig.h"

// Power management
static const uint32_t INACTIVITY_SLEEP_MS = 120000;  // 2 minutes

void simpleClickHandler(Button2 &btn);
void doubleClickHandler(Button2 &btn);
void longClickHandler(Button2 &btn);
void enterDeepSleep(const char* reason);
void markActivity();
void handleSleepLogic();

// Services
ButtonService buttonService;
FeedingService feedingService;
ClockService clockService;
ConfigService configService;
VibrationService vibrationService;
SchedulingService schedulingService(configService, clockService, feedingService);
WebService webService(configService, clockService, feedingService, schedulingService, vibrationService);

// State flags
bool wokeFromRtcAlarm = false;
bool wokeFromButton = false;
bool wokeFromPowerOn = false;
unsigned long lastActivityMillis = 0;
unsigned long ignoreButtonUntil = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n[INFO] ========================================");
  Serial.println("[INFO] Automatic Chicken Feeder v2.0");
  Serial.println("[INFO] ========================================\n");

  // Configure sleep callback for web API
  webService.setSleepCallback([]() { enterDeepSleep("Remote request"); });
  feedingService.setClockService(&clockService);
  feedingService.setConfigService(&configService);
  feedingService.setVibrationService(&vibrationService);

  // Wake cause detection
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  uint64_t gpioStatus = esp_sleep_get_gpio_wakeup_status();

  if (wakeupReason == ESP_SLEEP_WAKEUP_GPIO) {
    if (gpioStatus & (1ULL << RTC_INT_PIN)) {
      wokeFromRtcAlarm = true;
      Serial.println("[INFO] Wake reason: RTC alarm (GPIO)");
    }
    if (gpioStatus & (1ULL << BUTTON_PIN)) {
      wokeFromButton = true;
      Serial.println("[INFO] Wake reason: Button (GPIO)");
    }
  } else {
    wokeFromPowerOn = true;
    Serial.println("[INFO] Wake reason: Power-on reset or unknown");
  }

  // Check for maintenance mode (button held during boot)
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(50);  // Debounce
  bool maintenanceModeRequested = (digitalRead(BUTTON_PIN) == LOW);

  markActivity();

  // Initialize config service
  if (!configService.begin()) {
    Serial.println("[ERROR] Failed to initialize ConfigService!");
  }

  // Initialize clock service
  if (!clockService.begin()) {
    Serial.println("[WARN] DS3231 RTC not available - time sync required");
  }

  // Load feed history from persistent storage
  FeedHistoryEntry history[MAX_FEED_HISTORY];
  uint8_t historyWriteIndex = 0;
  uint8_t historyCount = configService.loadFeedHistory(history, MAX_FEED_HISTORY, historyWriteIndex);
  if (historyCount > 0) {
    feedingService.loadFeedHistory(history, historyCount, historyWriteIndex);
  }

  // Initialize button service
  buttonService.begin();
  buttonService.setSimpleClickHandler(simpleClickHandler);
  buttonService.setDoubleClickHandler(doubleClickHandler);
  buttonService.setLongClickHandler(longClickHandler);

  // Initialize feeding service
  feedingService.setup();

  // Initialize scheduling service
  schedulingService.begin();

  // Start web server
  if (!webService.begin()) {
    Serial.println("[ERROR] Failed to start WebService!");
  }

  // MAINTENANCE MODE CHECK
  if (maintenanceModeRequested) {
    Serial.println("\n[MAINTENANCE] ========================================");
    Serial.println("[MAINTENANCE] Entering MAINTENANCE MODE");
    Serial.println("[MAINTENANCE] OTA updates enabled");
    Serial.println("[MAINTENANCE] WiFi AP will stay active");
    Serial.println("[MAINTENANCE] No automatic sleep");
    Serial.println("[MAINTENANCE] ========================================\n");

    // Enable maintenance mode in WebService
    webService.setMaintenanceMode(true);

    // Start AP mode for OTA access
    webService.startAP("ChickenFeeder", "");

    // Infinite loop for maintenance mode - no sleep
    Serial.println("[MAINTENANCE] Ready for OTA updates");
    Serial.println("[MAINTENANCE] Connect to WiFi: ChickenFeeder");
    Serial.println("[MAINTENANCE] Open browser: http://192.168.4.1");
    Serial.println("[MAINTENANCE] Navigate to Maintenance section for firmware upload\n");

    while (true) {
      webService.update();
      delay(10);
    }
  }

  // Handle wake-specific actions
  if (wokeFromRtcAlarm) {
    // Process due events immediately (DS3231 alarm line stays low until cleared)
    schedulingService.checkAlarm();
  }

  if (wokeFromButton) {
    Serial.println("[BUTTON] Woke from button - AP will start shortly");

    // Ignore button events for 2 seconds to avoid consuming wakeup press
    ignoreButtonUntil = millis() + 2000;

    // Wait a short moment before starting AP (give button time to settle;
    // Button2's own debounce is only 50ms, so this just needs a small margin)
    delay(150);

    // Start AP mode
    Serial.println("[BUTTON] Starting AP mode");
    webService.startAP("ChickenFeeder", "");

    // Mark activity to prevent immediate sleep
    markActivity();
  }

  if (wokeFromPowerOn) {
    Serial.println("[BOOT] Cold boot - starting AP mode automatically");
    webService.startAP("ChickenFeeder", "");
    markActivity();
  }

  Serial.println("\n[INFO] Setup complete.");
  Serial.println("[INFO] Press button once to start AP mode");
  Serial.println("[INFO] Press button twice to feed manually");
  Serial.println("[INFO] Long press button to enter deep sleep");
  Serial.println("[INFO] OTA updates available via web interface when AP is active");
}

void loop() {
  buttonService.loop();
  feedingService.update();
  webService.update();
  schedulingService.update();
  vibrationService.update();
  handleSleepLogic();
}

void simpleClickHandler(Button2 &btn) {
  // Always mark activity (reset sleep timer), even if ignoring the click action
  markActivity();

  // Ignore button events if we're in the ignore window (after wakeup)
  if (millis() < ignoreButtonUntil) {
    Serial.println("[BUTTON] Ignoring single click (too soon after wakeup) - but timer reset");
    return;
  }

  Serial.println("[BUTTON] Single click - Starting AP mode");
  webService.startAP("ChickenFeeder", "");
}

void doubleClickHandler(Button2 &btn) {
  // Always mark activity (reset sleep timer), even if ignoring the click action
  markActivity();

  // Ignore button events if we're in the ignore window (after wakeup)
  if (millis() < ignoreButtonUntil) {
    Serial.println("[BUTTON] Ignoring double click (too soon after wakeup) - but timer reset");
    return;
  }

  Serial.println("[BUTTON] Double click - Manual feed");
  feedingService.feed(configService.getManualPortionUnits());
}

void longClickHandler(Button2 &btn) {
  Serial.println("[BUTTON] Long click - Entering deep sleep");
  enterDeepSleep("Manual long press");
}

void markActivity() {
  lastActivityMillis = millis();
}

void handleSleepLogic() {
  unsigned long now = millis();

  // Skip sleeping while feeding
  if (feedingService.isFeeding()) {
    markActivity();
    return;
  }

  // Skip sleeping while the post-feed vibration tail is still running
  // (feed can finish and isFeeding() go false before the tail winds down)
  if (vibrationService.isActive()) {
    markActivity();
    return;
  }

  // Keep awake while AP is active
  if (webService.isAPActive()) {
    return;
  }

  // After RTC alarm wake, go back to sleep once work is done
  if (wokeFromRtcAlarm) {
    enterDeepSleep("RTC alarm handled");
    return;
  }

  // Inactivity-based sleep
  uint32_t lastClient = webService.getLastClientActivity();
  uint32_t lastActivity = max(lastActivityMillis, lastClient);

  if ((now - lastActivity) >= INACTIVITY_SLEEP_MS) {
    enterDeepSleep("Inactivity timeout");
  }
}

void enterDeepSleep(const char* reason) {
  Serial.printf("[SLEEP] Entering deep sleep: %s\n", reason);

  // Save feed history to persistent storage before sleeping
  uint8_t historyCount = feedingService.getFeedHistoryCount();
  if (historyCount > 0) {
    configService.saveFeedHistory(feedingService.getFeedHistory(), historyCount, feedingService.getFeedHistoryWriteIndex());
  }

  // Ensure AP is stopped if it was running
  webService.stopAP();

  // Configure wake sources (active-low alarm/button)
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  const uint64_t wakeMask = (1ULL << RTC_INT_PIN) | (1ULL << BUTTON_PIN);
  esp_deep_sleep_enable_gpio_wakeup(wakeMask, ESP_GPIO_WAKEUP_GPIO_LOW);

  Serial.flush();
  esp_deep_sleep_start();
}
