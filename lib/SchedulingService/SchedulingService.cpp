#include "SchedulingService.hpp"

SchedulingService::SchedulingService(ConfigService &config, ClockService &clock, FeedingService &feeding)
    : configService(config), clockService(clock), feedingService(feeding), timerEventCount(0) {
}

void SchedulingService::begin() {
    Serial.println("[SCHED] SchedulingService initialized");

    // Generate initial timer events from config
    generateTimerEvents();

    // Program first alarm
    programNextAlarm();
}

void SchedulingService::update() {
    // Check if RTC alarm was triggered
    if (clockService.checkAlarmFlag()) {
        Serial.println("[SCHED] RTC alarm triggered!");
        checkAlarm();
    }
}

void SchedulingService::onConfigChanged() {
    Serial.println("[SCHED] Configuration changed - regenerating timers");

    // Regenerate all timer events
    generateTimerEvents();

    // Program next alarm
    programNextAlarm();
}

void SchedulingService::checkAlarm() {
    // Clear RTC alarm flag
    clockService.clearAlarm();

    // Find and handle all due events
    TimerEvent *event = getNextDueEvent();
    while (event != nullptr) {
        handleTimerEvent(*event);
        event->valid = false;  // Mark as done
        event = getNextDueEvent();
    }

    // Program next alarm
    programNextAlarm();
}

void SchedulingService::generateTimerEvents() {
    Serial.println("[SCHED] Generating timer events...");

    // Clear existing events
    timerEventCount = 0;
    for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
        timerEvents[i].valid = false;
    }

    DateTime now = clockService.now();

    // Load all schedules
    Schedule schedules[MAX_SCHEDULES];
    if (!configService.loadAllSchedules(schedules)) {
        Serial.println("[SCHED] Failed to load schedules");
        return;
    }

    // Generate events for next 7 days for each enabled schedule
    for (int schedIdx = 0; schedIdx < MAX_SCHEDULES; schedIdx++) {
        Schedule &sched = schedules[schedIdx];

        if (!sched.enabled) continue;

        // Try to find occurrences in next 7 days
        DateTime checkTime = now;
        for (int day = 0; day < 7 && timerEventCount < MAX_TIMER_EVENTS; day++) {
            DateTime nextOccurrence = getNextOccurrence(sched, checkTime);

            // Check if within next 7 days
            if (nextOccurrence.isValid() && nextOccurrence.unixtime() <= (now.unixtime() + 7 * 24 * 60 * 60)) {
                // Add to event list
                timerEvents[timerEventCount].timestamp = nextOccurrence;
                timerEvents[timerEventCount].scheduleId = sched.id;
                timerEvents[timerEventCount].portionUnits = sched.portion_units;
                timerEvents[timerEventCount].valid = true;
                timerEventCount++;

                Serial.printf("[SCHED] Added event: Schedule %d at %04d-%02d-%02d %02d:%02d\n",
                              sched.id,
                              nextOccurrence.year(), nextOccurrence.month(), nextOccurrence.day(),
                              nextOccurrence.hour(), nextOccurrence.minute());

                // Move to next day
                checkTime = DateTime(nextOccurrence.unixtime() + 24 * 60 * 60);
            } else {
                break;
            }
        }
    }

    Serial.printf("[SCHED] Generated %d timer events\n", timerEventCount);
}

DateTime SchedulingService::getNextOccurrence(const Schedule &schedule, const DateTime &from) {
    // Parse time from schedule (format: "HH:MM")
    int hour = (schedule.time[0] - '0') * 10 + (schedule.time[1] - '0');
    int minute = (schedule.time[3] - '0') * 10 + (schedule.time[4] - '0');

    // Start from today at the scheduled time
    DateTime candidate(from.year(), from.month(), from.day(), hour, minute, 0);

    // Allow a small grace window so alarms that just fired are still considered "due"
    const uint32_t GRACE_SECONDS = 60;
    if (candidate.unixtime() < (from.unixtime() - GRACE_SECONDS)) {
        // Time is more than the grace window in the past, move to tomorrow
        candidate = DateTime(candidate.unixtime() + 24 * 60 * 60);
    }

    // Find next day that matches weekday mask (max 7 days forward)
    for (int i = 0; i < 7; i++) {
        uint8_t weekday = candidate.dayOfTheWeek();  // 0=Sunday, 6=Saturday

        if (isActiveOnWeekday(schedule.weekday_mask, weekday)) {
            return candidate;
        }

        // Move to next day
        candidate = DateTime(candidate.unixtime() + 24 * 60 * 60);
    }

    // No valid occurrence found
    return DateTime((uint32_t)0);
}

bool SchedulingService::isActiveOnWeekday(uint8_t weekdayMask, uint8_t weekday) {
    // weekday: 0=Sunday, 1=Monday, ..., 6=Saturday
    // weekdayMask: bit 0=Sunday, bit 1=Monday, ..., bit 6=Saturday
    return (weekdayMask & (1 << weekday)) != 0;
}

TimerEvent* SchedulingService::getNextFutureEvent() {
    DateTime now = clockService.now();
    TimerEvent *best = nullptr;

    for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
        if (!timerEvents[i].valid) continue;
        if (timerEvents[i].timestamp.unixtime() <= now.unixtime()) continue;

        if (best == nullptr || timerEvents[i].timestamp.unixtime() < best->timestamp.unixtime()) {
            best = &timerEvents[i];
        }
    }

    return best;
}

TimerEvent* SchedulingService::getNextDueEvent() {
    DateTime now = clockService.now();
    TimerEvent *best = nullptr;

    for (int i = 0; i < MAX_TIMER_EVENTS; i++) {
        if (!timerEvents[i].valid) continue;
        if (timerEvents[i].timestamp.unixtime() > now.unixtime()) continue;

        if (best == nullptr || timerEvents[i].timestamp.unixtime() < best->timestamp.unixtime()) {
            best = &timerEvents[i];
        }
    }

    return best;
}

void SchedulingService::programNextAlarm() {
    TimerEvent *next = getNextFutureEvent();

    if (next == nullptr) {
        Serial.println("[SCHED] No future events - alarm disabled");
        clockService.clearAlarm();
        return;
    }

    // Program RTC alarm for next event
    if (clockService.setAlarm(next->timestamp)) {
        Serial.printf("[SCHED] Next alarm programmed for schedule %d\n", next->scheduleId);
    }
}

void SchedulingService::handleTimerEvent(TimerEvent &event) {
    Serial.printf("[SCHED] Executing timer event: Schedule %d, %d portions\n",
                  event.scheduleId, event.portionUnits);

    // Trigger feeding
    feedingService.feed(event.portionUnits);
}
