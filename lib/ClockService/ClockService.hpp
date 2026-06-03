#ifndef CLOCK_SERVICE_HPP
#define CLOCK_SERVICE_HPP

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

class ClockService {
public:
    ClockService();

    bool begin();
    bool isAvailable();

    // Time management
    bool setTime(uint32_t unixTime);
    DateTime now();
    int32_t getCurrentUtcOffsetSeconds();
    const char* getCurrentTimeZoneName();

    // Check if RTC needs sync (drift > threshold)
    bool needsSync(uint32_t thresholdMs = 3000);

    // Alarm management
    bool setAlarm(const DateTime &dt);
    bool clearAlarm();
    bool checkAlarmFlag();

private:
    RTC_DS3231 rtc;
    bool available;
    uint32_t lastSyncTime;

    static uint8_t lastSundayOfMonth(uint16_t year, uint8_t month);
    static bool isDstEuropeBerlinFromUtc(const DateTime &utcTime);
    static bool isDstEuropeBerlinLocal(const DateTime &localTime);
};

#endif // CLOCK_SERVICE_HPP
