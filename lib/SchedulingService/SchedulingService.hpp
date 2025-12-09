#ifndef SCHEDULING_SERVICE_HPP
#define SCHEDULING_SERVICE_HPP

#include <Arduino.h>
#include <RTClib.h>
#include "ConfigService.hpp"
#include "ClockService.hpp"
#include "FeedingService.hpp"

#define MAX_TIMER_EVENTS 50  // Max events in queue (e.g., week of schedules)

struct TimerEvent {
    DateTime timestamp;
    uint8_t scheduleId;  // Which schedule triggered this
    uint8_t portionUnits;
    bool valid;
};

class SchedulingService {
public:
    SchedulingService(ConfigService &config, ClockService &clock, FeedingService &feeding);

    void begin();
    void update();

    // Called when configuration changes
    void onConfigChanged();

    // Check for alarm trigger (call from loop or ISR flag)
    void checkAlarm();

private:
    ConfigService &configService;
    ClockService &clockService;
    FeedingService &feedingService;

    TimerEvent timerEvents[MAX_TIMER_EVENTS];
    int timerEventCount;

    // Calculate next week of timer events from schedules
    void generateTimerEvents();

    // Find next future event
    TimerEvent* getNextFutureEvent();

    // Find next due event (now or past)
    TimerEvent* getNextDueEvent();

    // Program the next alarm on RTC
    void programNextAlarm();

    // Handle a triggered timer event
    void handleTimerEvent(TimerEvent &event);

    // Calculate next occurrence of a schedule
    DateTime getNextOccurrence(const Schedule &schedule, const DateTime &from);

    // Check if schedule should run on given weekday
    bool isActiveOnWeekday(uint8_t weekdayMask, uint8_t weekday);
};

#endif // SCHEDULING_SERVICE_HPP
