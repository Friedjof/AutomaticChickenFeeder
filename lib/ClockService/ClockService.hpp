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

    // Check if RTC needs sync (drift > threshold)
    bool needsSync(uint32_t thresholdMs = 3000);

private:
    RTC_DS3231 rtc;
    bool available;
    uint32_t lastSyncTime;
};

#endif // CLOCK_SERVICE_HPP
