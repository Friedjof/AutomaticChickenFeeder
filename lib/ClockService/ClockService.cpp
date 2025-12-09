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

    DateTime newTime(unixTime);
    rtc.adjust(newTime);
    lastSyncTime = millis();

    Serial.printf("[CLOCK] Time updated to: %04d-%02d-%02d %02d:%02d:%02d\n",
                  newTime.year(), newTime.month(), newTime.day(),
                  newTime.hour(), newTime.minute(), newTime.second());

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
