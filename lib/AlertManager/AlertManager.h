#ifndef __DEFINE_ALERT_MANAGER_H__
#define __DEFINE_ALERT_MANAGER_H__

#include <Arduino.h>
#include <unordered_map>
#include <string>

#include <ClockService.h>
#include <ConfigManager.h>
#include <LoggingManager.h>

// Interrupt frequency, in seconds
#define ALERT_BITS 0b00010000 // Alarm when day, hours, minutes, and seconds match

// type definitions
typedef struct
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;

    unsigned int weekday;

    float temperature;
} ds3231_datetime_t;

typedef struct
{
    int hour;
    int minute;
    int weekday;

    // optional timer id
    int optional_id;
} ds3231_timer_t;

typedef struct
{
    ds3231_timer_t *timers;
    size_t num_timers;
} ds3231_timer_list_t;

typedef struct
{
    ds3231_timer_t timer;
    bool empty;
} optional_ds3231_timer_t;

class AlertManager
{
private:
    ConfigManager &configManager;
    ClockService &clockService;
    LoggingManager &loggingManager;

    byte alarmDay, alarmHour, alarmMinute, alarmSecond, alarmBits;

    bool initialized = false;

public:
    AlertManager(ConfigManager &configManager, LoggingManager &loggingManager, ClockService &clockService);
    ~AlertManager();

    void begin();

    // debugging functions
    void print_now();
    void print_temperature();
    void print_timer(ds3231_timer_t timer);
    void print_timer(timer_config_t timer);
    void print_timer_list(ds3231_timer_list_t timers);

    ds3231_datetime_t now();

    void set_next_alert();
    void set_alert(DateTime alert);
    optional_ds3231_timer_t get_next_alert();
    ds3231_timer_list_t convert_to_timer_list(timer_config_list_t timers);
    int get_next_weekday_from_timer(timer_config_t timer, int current_weekday);
    optional_timer_config_list_t get_timers_by_weekday(int weekday, timer_config_list_t timers);
    optional_ds3231_timer_t get_earliest_timer_of_the_day(timer_config_list_t timers, int weekday);
    bool timer_is_active_on_weekday(timer_config_t timer, int weekday);

    bool set_new_datetime(int year, int month, int day, int hour, int minute, int second);

    void setup_interrupt();
    void disable_alarm_2();

    int weekday_to_int(char *weekday);
    String int_to_weekday(int weekday);
};

#endif