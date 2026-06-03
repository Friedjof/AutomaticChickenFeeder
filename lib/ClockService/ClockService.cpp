#include "ClockService.hpp"

ClockService::ClockService() : available(false), lastSyncTime(0) {}

uint8_t ClockService::lastSundayOfMonth(uint16_t year, uint8_t month) {
    DateTime firstOfNextMonth = month == 12
        ? DateTime(year + 1, 1, 1, 0, 0, 0)
        : DateTime(year, month + 1, 1, 0, 0, 0);
    DateTime lastOfMonth(firstOfNextMonth.unixtime() - 24 * 60 * 60);
    return lastOfMonth.day() - lastOfMonth.dayOfTheWeek();
}

bool ClockService::isDstEuropeBerlinFromUtc(const DateTime &utcTime) {
    const uint8_t month = utcTime.month();
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;

    const uint8_t lastSunday = lastSundayOfMonth(utcTime.year(), month);
    if (month == 3) {
        return utcTime.day() > lastSunday ||
               (utcTime.day() == lastSunday && utcTime.hour() >= 1);
    }

    return utcTime.day() < lastSunday ||
           (utcTime.day() == lastSunday && utcTime.hour() < 1);
}

bool ClockService::isDstEuropeBerlinLocal(const DateTime &localTime) {
    const uint8_t month = localTime.month();
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;

    const uint8_t lastSunday = lastSundayOfMonth(localTime.year(), month);
    if (month == 3) {
        return localTime.day() > lastSunday ||
               (localTime.day() == lastSunday && localTime.hour() >= 3);
    }

    return localTime.day() < lastSunday ||
           (localTime.day() == lastSunday && localTime.hour() < 3);
}

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

    const DateTime utcTime(unixTime);
    const bool isDst = isDstEuropeBerlinFromUtc(utcTime);
    const int32_t timezoneOffset = isDst ? 7200 : 3600;
    DateTime localTime(unixTime + timezoneOffset);
    rtc.adjust(localTime);
    lastSyncTime = millis();

    Serial.printf("[CLOCK] Time set to: %04d-%02d-%02d %02d:%02d:%02d (%s)\n",
                  localTime.year(), localTime.month(), localTime.day(),
                  localTime.hour(), localTime.minute(), localTime.second(),
                  isDst ? "CEST" : "CET");

    return true;
}

DateTime ClockService::now() {
    if (!available) {
        // Return epoch time if RTC not available
        return DateTime((uint32_t)0);
    }
    return rtc.now();
}

int32_t ClockService::getCurrentUtcOffsetSeconds() {
    if (!available) return 0;
    return isDstEuropeBerlinLocal(rtc.now()) ? 7200 : 3600;
}

const char* ClockService::getCurrentTimeZoneName() {
    if (!available) return "UTC";
    return isDstEuropeBerlinLocal(rtc.now()) ? "CEST" : "CET";
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
