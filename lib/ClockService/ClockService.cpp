#include "ClockService.hpp"

ClockService::ClockService() : available(false), lastSyncTime(0) {}

bool ClockService::begin() {
    if (!rtc.begin()) {
        Serial.println("[CLOCK] DS3231 not found!");
        available = false;
        return false;
    }

    if (rtc.lostPower()) {
        Serial.println("[CLOCK] RTC lost power, needs time sync!");
    }

    available = true;
    lastSyncTime = millis();

    DateTime now = rtc.now();
    Serial.printf("[CLOCK] DS3231 initialized. Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());

    return true;
}

bool ClockService::isAvailable() {
    return available;
}

bool ClockService::setTime(uint32_t unixTime) {
    if (!available) {
        Serial.println("[CLOCK] RTC not available!");
        return false;
    }

    // Convert UTC to Europe/Berlin timezone (CET/CEST)
    // Winter (CET): UTC+1 = +3600 seconds
    // Summer (CEST): UTC+2 = +7200 seconds
    // For simplicity, using fixed CET offset (UTC+1)
    const int32_t TIMEZONE_OFFSET = 3600; // +1 hour for CET (winter)

    DateTime localTime(unixTime + TIMEZONE_OFFSET);
    rtc.adjust(localTime);
    lastSyncTime = millis();

    Serial.printf("[CLOCK] Time set to: %04d-%02d-%02d %02d:%02d:%02d (UTC+1)\n",
                  localTime.year(), localTime.month(), localTime.day(),
                  localTime.hour(), localTime.minute(), localTime.second());

    return true;
}

DateTime ClockService::now() {
    if (!available) {
        // Return epoch time if RTC not available
        return DateTime((uint32_t)0);
    }
    return rtc.now();
}

bool ClockService::needsSync(uint32_t thresholdMs) {
    if (!available) return true;
    return (millis() - lastSyncTime) > thresholdMs;
}

bool ClockService::setAlarm(const DateTime &dt) {
    if (!available) {
        Serial.println("[CLOCK] Cannot set alarm - RTC not available");
        return false;
    }

    // Clear any existing alarm
    rtc.clearAlarm(1);

    // Set Alarm1 to match date, hour, minute, second
    if (!rtc.setAlarm1(dt, DS3231_A1_Date)) {
        Serial.println("[CLOCK] Failed to set alarm");
        return false;
    }

    Serial.printf("[CLOCK] Alarm set for: %04d-%02d-%02d %02d:%02d:%02d\n",
                  dt.year(), dt.month(), dt.day(),
                  dt.hour(), dt.minute(), dt.second());

    return true;
}

bool ClockService::clearAlarm() {
    if (!available) return false;

    rtc.clearAlarm(1);
    rtc.disableAlarm(1);
    Serial.println("[CLOCK] Alarm cleared");

    return true;
}

bool ClockService::checkAlarmFlag() {
    if (!available) return false;

    return rtc.alarmFired(1);
}
