#include "AlertManager.h"

AlertManager::AlertManager(ConfigManager configManager) {
    this->configManager = configManager;
}

AlertManager::AlertManager() {
    this->configManager = ConfigManager();
}

AlertManager::~AlertManager() { }

// Setup
void AlertManager::setup() {
    // Setup the I2C bus (SDA = GPIO21, SCL = GPIO22) for the RTC DS3231
    Wire.begin();

    // Setup der RTC
    this->rtc.setClockMode(false); // Set clock mode to 24 hour

    // Setup next alert
    this->set_next_alert();
}

void AlertManager::set_next_alert() {
    this->configManager.load_config();

    optional_ds3231_timer_t next_alert = this->get_next_alert();

    if (!next_alert.empty) {
        this->set_alert(this->convert_to_rtc_alert(next_alert.timer));
    } else {
        Serial.println("No next alert found!");
    }
}

// helper functions
ds3231_datetime_t AlertManager::now() {
    ds3231_datetime_t now;
    now.year = (2000 + this->century * 100) + this->rtc.getYear();
    now.month = this->rtc.getMonth(this->century);
    now.day = this->rtc.getDate();
    now.hour = this->rtc.getHour(this->h12Flag, this->pmFlag);
    now.minute = this->rtc.getMinute();
    now.second = this->rtc.getSecond();
    now.weekday = this->rtc.getDoW();

    now.temperature = this->rtc.getTemperature();
    return now;
}

int AlertManager::weekday_to_int(char *weekday) {
        if (strcmp(weekday, "mon") == 0)
        {
            return 1;
        }
        else if (strcmp(weekday, "tue") == 0)
        {
            return 2;
        }
        else if (strcmp(weekday, "wed") == 0)
        {
            return 3;
        }
        else if (strcmp(weekday, "thu") == 0)
        {
            return 4;
        }
        else if (strcmp(weekday, "fri") == 0)
        {
            return 5;
        }
        else if (strcmp(weekday, "sat") == 0)
        {
            return 6;
        }
        else if (strcmp(weekday, "son") == 0)
        {
            return 7;
        }
        else
        {
            return 0;
        }
}

String AlertManager::int_to_weekday(int weekday) {
    switch (weekday) {
        case 1:
            return "mon";
        case 2:
            return "tue";
        case 3:
            return "wed";
        case 4:
            return "thu";
        case 5:
            return "fri";
        case 6:
            return "sat";
        case 7:
            return "son";
        default:
            return "mon";
    }
}

optional_ds3231_timer_t AlertManager::get_next_alert() {
    ds3231_datetime_t now = this->now();
    int current_weekday = now.weekday;

    optional_ds3231_timer_t optional_earliest_timer;
    optional_earliest_timer.empty = true; // default to true (no timer found)

    ds3231_timer_t earliest_timer;

    timer_config_list_t timers = this->configManager.get_timers();

    for (int i = current_weekday - 1; i <= current_weekday + 6; i++) {
        int weekday = (i % 7) + 1;
        optional_timer_config_list_t timers_by_weekday = this->get_timers_by_weekday(weekday, timers);

        if (!timers_by_weekday.empty) {
            if (weekday == current_weekday) {
                timer_config_list_t sorted_timers = this->configManager.sort_timers_by_time(timers_by_weekday.timers);

                timer_search:
                // Suche nach der Uhrzeit nach dem aktuellen Zeitpunkt
                for (size_t j = 0; j < sorted_timers.num_timers; j++) {
                    timer_config_t next_alert = sorted_timers.timers[j];

                    if (next_alert.time.hour > now.hour) {
                        earliest_timer.hour = next_alert.time.hour;
                        earliest_timer.minute = next_alert.time.minute;
                        earliest_timer.weekday = weekday;

                        optional_earliest_timer.empty = false;
                        optional_earliest_timer.timer = earliest_timer;

                        // break the two loops
                        i = current_weekday + 6;
                        break;
                    } else if (next_alert.time.hour == now.hour && next_alert.time.minute > now.minute) {
                        earliest_timer.hour = next_alert.time.hour;
                        earliest_timer.minute = next_alert.time.minute;
                        earliest_timer.weekday = weekday;

                        optional_earliest_timer.empty = false;
                        optional_earliest_timer.timer = earliest_timer;

                        // break the two loops
                        i = current_weekday + 6;
                        break;
                    }
                }

                free(sorted_timers.timers);
            } else {
                optional_earliest_timer = this->get_earliest_timer_of_the_day(timers_by_weekday.timers, weekday);
                break;
            }
        }
    }

    if (!optional_earliest_timer.empty) {
        return optional_earliest_timer;
    }

    // Deaktiviert den Alarm, wenn kein Timer aktiv ist
    this->rtc.turnOffAlarm(1);

    ds3231_timer_t next_alert;
    next_alert.hour = 0;
    next_alert.minute = 0;
    next_alert.weekday = 0;

    optional_ds3231_timer_t optional_next_alert;
    optional_next_alert.empty = true;
    optional_next_alert.timer = next_alert;

    return optional_next_alert;
}

int AlertManager::get_next_weekday_from_timer(timer_config_t timer, int current_weekday) {
    int next_weekday = current_weekday;

    if (timer.monday && next_weekday <= 1) {
        next_weekday = 1;
    } else if (timer.tuesday && next_weekday <= 2) {
        next_weekday = 2;
    } else if (timer.wednesday && next_weekday <= 3) {
        next_weekday = 3;
    } else if (timer.thursday && next_weekday <= 4) {
        next_weekday = 4;
    } else if (timer.friday && next_weekday <= 5) {
        next_weekday = 5;
    } else if (timer.saturday && next_weekday <= 6) {
        next_weekday = 6;
    } else if (timer.sunday && next_weekday <= 7) {
        next_weekday = 7;
    }

    return next_weekday;
}

optional_timer_config_list_t AlertManager::get_timers_by_weekday(int weekday, timer_config_list_t timers) {
    optional_timer_config_list_t optional_timers_by_weekday;
    optional_timers_by_weekday.empty = true;

    timer_config_list_t timers_by_weekday;
    timers_by_weekday.timers = (timer_config_t*) malloc(sizeof(timer_config_t) * timers.num_timers);
    timers_by_weekday.num_timers = 0;

    for (size_t i = 0; i < timers.num_timers; i++) {
        timer_config_t timer = timers.timers[i];

        if (timer.enabled) {
            if (timer.monday && weekday == 1) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            } else if (timer.tuesday && weekday == 2) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            } else if (timer.wednesday && weekday == 3) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            } else if (timer.thursday && weekday == 4) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            } else if (timer.friday && weekday == 5) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            } else if (timer.saturday && weekday == 6) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            } else if (timer.sunday && weekday == 7) {
                timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
                timers_by_weekday.num_timers++;
            }
        }
    }

    if (timers_by_weekday.num_timers > 0) {
        optional_timers_by_weekday.empty = false;
    }

    optional_timers_by_weekday.timers = timers_by_weekday;

    return optional_timers_by_weekday;
}

optional_ds3231_timer_t AlertManager::get_earliest_timer_of_the_day(timer_config_list_t timers, int weekday) {
    optional_ds3231_timer_t optional_earliest_timer;
    optional_earliest_timer.empty = true;

    ds3231_timer_t earliest_timer;
    earliest_timer.hour = 23;
    earliest_timer.minute = 59;
    earliest_timer.weekday = 0;

    for (size_t i = 0; i < timers.num_timers; i++) {
        timer_config_t timer = timers.timers[i];

        if (timer.enabled && this->timer_is_active_on_weekday(timer, weekday)) {
            timer_time_t timer_time = timer.time;

            ds3231_timer_t next_alert;
            next_alert.hour = timer_time.hour;
            next_alert.minute = timer_time.minute;
            next_alert.weekday = weekday;

            if (next_alert.hour < earliest_timer.hour) {
                earliest_timer = next_alert;
                optional_earliest_timer.empty = false;
            } else if (next_alert.hour == earliest_timer.hour && next_alert.minute < earliest_timer.minute) {
                earliest_timer = next_alert;
                optional_earliest_timer.empty = false;
            }
        }
    }

    optional_earliest_timer.timer = earliest_timer;

    return optional_earliest_timer;
}

ds3231_timer_list_t AlertManager::convert_to_timer_list(timer_config_list_t timers) {
    ds3231_timer_list_t timer_list;
    timer_list.timers = (ds3231_timer_t*) malloc(sizeof(ds3231_timer_t) * timers.num_timers);
    timer_list.num_timers = 0;

    for (size_t i = 0; i < timers.num_timers; i++) {
        timer_config_t timer = timers.timers[i];

        ds3231_timer_t next_alert;
        next_alert.hour = timer.time.hour;
        next_alert.minute = timer.time.minute;
        next_alert.weekday = this->get_next_weekday_from_timer(timer, this->now().weekday);

        timer_list.timers[timer_list.num_timers] = next_alert;
        timer_list.num_timers++;
    }

    return timer_list;
}

rtc_alert_t AlertManager::convert_to_rtc_alert(ds3231_timer_t timer) {
    rtc_alert_t alert;
    alert.day = timer.weekday;
    alert.hour = timer.hour;
    alert.minute = timer.minute;
    alert.second = 0;
    alert.alert_bits = ALERT_BITS; // Alarm when seconds match
    alert.day_is_day = true;       // Use day of the week and not day of the month
    alert.h12 = false;             // Use 24 hour format
    alert.pm = false;              // Ignore AM/PM
    alert.alarm_flag = 0;          // Reset the alarm flag

    return alert;
}

void AlertManager::set_alert(rtc_alert_t alert) {
    // Setze den Alarm
    this->rtc.turnOffAlarm(1);
    this->rtc.setA1Time(
        alert.day, alert.hour, alert.minute, alert.second,
        alert.alert_bits, alert.day_is_day, alert.h12, alert.pm
    );
    this->rtc.checkIfAlarm(1);
    this->rtc.turnOnAlarm(1);

    // Debugging
    Serial.print("next alert: ");
    Serial.printf("%02d", alert.hour);
    Serial.print(":");
    Serial.printf("%02d", alert.minute);
    Serial.print(" ");
    Serial.println(this->int_to_weekday(alert.day));

    // Aktiviere den Alarm
    //this->rtc.enableInterrupts(false);
}

void AlertManager::disable_alarm_2() {
    this->rtc.setA2Time(0x00, 0x00, 0xff, 0x60, false, false, false); // 0x60 = 0b01100000 => Alarm when minutes match (never because of 0xff)
    this->rtc.turnOffAlarm(2);
    this->rtc.checkIfAlarm(2);
}

bool AlertManager::timer_is_active_on_weekday(timer_config_t timer, int weekday) {
    if (timer.enabled) {
        if (timer.monday && weekday == 1) {
            return true;
        } else if (timer.tuesday && weekday == 2) {
            return true;
        } else if (timer.wednesday && weekday == 3) {
            return true;
        } else if (timer.thursday && weekday == 4) {
            return true;
        } else if (timer.friday && weekday == 5) {
            return true;
        } else if (timer.saturday && weekday == 6) {
            return true;
        } else if (timer.sunday && weekday == 7) {
            return true;
        }
    }

    return false;
}

// Debugging
void AlertManager::print_now() {
	ds3231_datetime_t now = this->now();

    // Display the date
    Serial.print(now.year);
    Serial.print("-");
    Serial.printf("%02d", now.month);
    Serial.print("-");
    Serial.printf("%02d", now.day);
    Serial.print(" ");

    // Display the time
    Serial.printf("%02d", now.hour);
    Serial.print(":");
    Serial.printf("%02d", now.minute);
    Serial.print(":");
    Serial.printf("%02d", now.second);
    Serial.print(" ");
    Serial.println();
}

void AlertManager::print_temperature() {
    Serial.print("Temperatur: ");
    Serial.print(this->now().temperature);
    Serial.println("°C");
}

void AlertManager::print_timer(ds3231_timer_t timer) {
    Serial.print("Timer: ");
    Serial.printf("%02d", timer.hour);
    Serial.print(":");
    Serial.printf("%02d", timer.minute);
    Serial.print(" ");
    Serial.println(this->int_to_weekday(timer.weekday));
}

void AlertManager::print_timer(timer_config_t timer) {
    Serial.print("Timer: ");
    Serial.printf("%02d", timer.time.hour);
    Serial.print(":");
    Serial.printf("%02d", timer.time.minute);
    Serial.print(" ");
    Serial.println(this->int_to_weekday(this->get_next_weekday_from_timer(timer, this->now().weekday)));
}

void AlertManager::print_timer_list(ds3231_timer_list_t timers) {
    Serial.print("[ ");
    for (size_t i = 0; i < timers.num_timers; i++) {
        ds3231_timer_t timer = timers.timers[i];

        Serial.print("{ ");
        Serial.printf("%02d", timer.hour);
        Serial.print(":");
        Serial.printf("%02d", timer.minute);
        Serial.print(", '");
        Serial.print(this->int_to_weekday(timer.weekday));
        Serial.print("' }");

        if (i < timers.num_timers - 1) {
            Serial.print(", ");
        }
    }
    Serial.println(" ]");
}
