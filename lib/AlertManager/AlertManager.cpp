#include "AlertManager.h"

AlertManager::AlertManager(
    ConfigManager &configManager, LoggingManager &loggingManager, ClockService &clockService) : configManager(configManager), loggingManager(loggingManager), clockService(clockService) {}

AlertManager::~AlertManager() {}

// Setup
void AlertManager::begin()
{
  Wire.begin();

  // Setup next alert
  this->set_next_alert();

  this->initialized = true;
}

void AlertManager::set_next_alert()
{
  this->configManager.load_config();

  optional_ds3231_timer_t next_alert = this->get_next_alert();

  if (!next_alert.empty)
  {
    this->set_alert(this->convert_to_rtc_alert(next_alert.timer));

    loggingManager.start_seq(LOG_LEVEL_INFO, "next timer id: ");
    loggingManager.append_seq(next_alert.timer.optional_id);
    loggingManager.end_seq();

    this->configManager.set_next_timer_id(next_alert.timer.optional_id);
    this->configManager.save_config();
  }
  else
  {
    loggingManager.log(LOG_LEVEL_WARNING, "no next alert found!");
  }
}

ds3231_datetime_t AlertManager::now()
{
  ds3231_datetime_t now;
  now.year = this->clockService.getYear();
  now.month = this->clockService.getMonth();
  now.day = this->clockService.getDay();
  now.hour = this->clockService.getHour();
  now.minute = this->clockService.getMinute();
  now.second = this->clockService.getSecond();
  now.weekday = this->clockService.getDoW();

  now.temperature = this->clockService.getTemperature();
  return now;
}

bool AlertManager::set_new_datetime(int year, int month, int day, int hour, int minute, int second)
{
  this->clockService.set_datetime(year, month, day, hour, minute, second);

  return true;
}

int AlertManager::weekday_to_int(char *weekday)
{
  std::unordered_map<std::string, int> weekdays = {
      {"mon", 1},
      {"tue", 2},
      {"wed", 3},
      {"thu", 4},
      {"fri", 5},
      {"sat", 6},
      {"son", 7}};

  auto it = weekdays.find(weekday);
  if (it != weekdays.end())
  {
    return it->second;
  }
  else
  {
    return 0;
  }
}

String AlertManager::int_to_weekday(int weekday)
{
  std::unordered_map<int, const char *> weekdays = {
      {1, "mon"},
      {2, "tue"},
      {3, "wed"},
      {4, "thu"},
      {5, "fri"},
      {6, "sat"},
      {7, "son"}};

  auto it = weekdays.find(weekday);
  if (it != weekdays.end())
  {
    return it->second;
  }
  else
  {
    return "unknown";
  }
}

optional_ds3231_timer_t AlertManager::get_next_alert()
{
  ds3231_datetime_t now = this->now();
  int current_weekday = now.weekday;

  optional_ds3231_timer_t optional_earliest_timer;
  optional_earliest_timer.empty = true; // default to true (no timer found)

  ds3231_timer_t earliest_timer;

  timer_config_list_t timers = this->configManager.get_timers();

  for (int i = current_weekday - 1; i <= current_weekday + 6; i++)
  {
    int weekday = (i % 7) + 1;
    optional_timer_config_list_t timers_by_weekday = this->get_timers_by_weekday(weekday, timers);

    if (!timers_by_weekday.empty)
    {
      if (weekday == current_weekday)
      {
        timer_config_list_t sorted_timers = this->configManager.sort_timers_by_time(timers_by_weekday.timers);

        // search for the next timer
        for (size_t j = 0; j < sorted_timers.num_timers; j++)
        {
          timer_config_t next_alert = sorted_timers.timers[j];

          if (next_alert.time.hour > now.hour)
          {
            earliest_timer.hour = next_alert.time.hour;
            earliest_timer.minute = next_alert.time.minute;
            earliest_timer.weekday = weekday;
            earliest_timer.optional_id = next_alert.optional_id;

            optional_earliest_timer.empty = false;
            optional_earliest_timer.timer = earliest_timer;

            // break the two loops
            i = current_weekday + 6;
            break;
          }
          else if (next_alert.time.hour == now.hour && next_alert.time.minute > now.minute)
          {
            earliest_timer.hour = next_alert.time.hour;
            earliest_timer.minute = next_alert.time.minute;
            earliest_timer.weekday = weekday;
            earliest_timer.optional_id = next_alert.optional_id;

            optional_earliest_timer.empty = false;
            optional_earliest_timer.timer = earliest_timer;

            // break the two loops
            i = current_weekday + 6;
            break;
          }
        }

        free(sorted_timers.timers);
      }
      else
      {
        optional_earliest_timer = this->get_earliest_timer_of_the_day(timers_by_weekday.timers, weekday);
        break;
      }
    }
  }

  if (!optional_earliest_timer.empty)
  {
    return optional_earliest_timer;
  }

  // deactivate the alarm if no timer is active
  this->clockService.turnOffAlarm(1);

  ds3231_timer_t next_alert;
  next_alert.hour = 0;
  next_alert.minute = 0;
  next_alert.weekday = 0;

  optional_ds3231_timer_t optional_next_alert;
  optional_next_alert.empty = true;
  optional_next_alert.timer = next_alert;

  return optional_next_alert;
}

int AlertManager::get_next_weekday_from_timer(timer_config_t timer, int current_weekday)
{
  int next_weekday = current_weekday;

  if (timer.monday && next_weekday <= 1)
  {
    next_weekday = 1;
  }
  else if (timer.tuesday && next_weekday <= 2)
  {
    next_weekday = 2;
  }
  else if (timer.wednesday && next_weekday <= 3)
  {
    next_weekday = 3;
  }
  else if (timer.thursday && next_weekday <= 4)
  {
    next_weekday = 4;
  }
  else if (timer.friday && next_weekday <= 5)
  {
    next_weekday = 5;
  }
  else if (timer.saturday && next_weekday <= 6)
  {
    next_weekday = 6;
  }
  else if (timer.sunday && next_weekday <= 7)
  {
    next_weekday = 7;
  }

  return next_weekday;
}

optional_timer_config_list_t AlertManager::get_timers_by_weekday(int weekday, timer_config_list_t timers)
{
  optional_timer_config_list_t optional_timers_by_weekday;
  optional_timers_by_weekday.empty = true;

  timer_config_list_t timers_by_weekday;
  timers_by_weekday.timers = (timer_config_t *)malloc(sizeof(timer_config_t) * timers.num_timers);
  timers_by_weekday.num_timers = 0;

  for (size_t i = 0; i < timers.num_timers; i++)
  {
    timer_config_t timer = timers.timers[i];
    timer.optional_id = i;

    if (timer.enabled)
    {
      if (timer.monday && weekday == 1)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
      else if (timer.tuesday && weekday == 2)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
      else if (timer.wednesday && weekday == 3)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
      else if (timer.thursday && weekday == 4)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
      else if (timer.friday && weekday == 5)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
      else if (timer.saturday && weekday == 6)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
      else if (timer.sunday && weekday == 7)
      {
        timers_by_weekday.timers[timers_by_weekday.num_timers] = timer;
        timers_by_weekday.num_timers++;
      }
    }
  }

  if (timers_by_weekday.num_timers > 0)
  {
    optional_timers_by_weekday.empty = false;
  }

  optional_timers_by_weekday.timers = timers_by_weekday;

  return optional_timers_by_weekday;
}

optional_ds3231_timer_t AlertManager::get_earliest_timer_of_the_day(timer_config_list_t timers, int weekday)
{
  optional_ds3231_timer_t optional_earliest_timer;
  optional_earliest_timer.empty = true;

  ds3231_timer_t earliest_timer;
  earliest_timer.hour = 23;
  earliest_timer.minute = 59;
  earliest_timer.weekday = 0;

  for (size_t i = 0; i < timers.num_timers; i++)
  {
    timer_config_t timer = timers.timers[i];

    if (timer.enabled && this->timer_is_active_on_weekday(timer, weekday))
    {
      timer_time_t timer_time = timer.time;

      ds3231_timer_t next_alert;
      next_alert.hour = timer_time.hour;
      next_alert.minute = timer_time.minute;
      next_alert.weekday = weekday;
      next_alert.optional_id = timer.optional_id;

      if (next_alert.hour < earliest_timer.hour)
      {
        earliest_timer = next_alert;
        optional_earliest_timer.empty = false;
      }
      else if (next_alert.hour == earliest_timer.hour && next_alert.minute < earliest_timer.minute)
      {
        earliest_timer = next_alert;
        optional_earliest_timer.empty = false;
      }
    }
  }

  optional_earliest_timer.timer = earliest_timer;

  return optional_earliest_timer;
}

ds3231_timer_list_t AlertManager::convert_to_timer_list(timer_config_list_t timers)
{
  ds3231_timer_list_t timer_list;
  timer_list.timers = (ds3231_timer_t *)malloc(sizeof(ds3231_timer_t) * timers.num_timers);
  timer_list.num_timers = 0;

  for (size_t i = 0; i < timers.num_timers; i++)
  {
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

rtc_alert_t AlertManager::convert_to_rtc_alert(ds3231_timer_t timer)
{
  rtc_alert_t alert;
  alert.day = timer.weekday;
  alert.hour = timer.hour;
  alert.minute = timer.minute;
  alert.second = 0;
  alert.alert_bits = ALERT_BITS; // See the header file for more information
  alert.day_is_day = true;       // Use day of the week and not day of the month
  alert.h12 = false;             // Use 24 hour format
  alert.pm = false;              // Ignore AM/PM
  alert.alarm_flag = 0;          // Reset the alarm flag

  return alert;
}

void AlertManager::set_alert(rtc_alert_t alert)
{
  // Set the alarm
  this->clockService.turnOffAlarm(1);
  this->clockService.setA1Time(
      alert.day, alert.hour, alert.minute, alert.second,
      alert.alert_bits, alert.day_is_day);
  this->clockService.checkIfAlarm(1);
  this->clockService.turnOnAlarm(1);

  // Print the new alert
  loggingManager.start_seq(LOG_LEVEL_INFO, "new alert: ");
  loggingManager.append_seq(alert.hour);
  loggingManager.append_seq(":");
  loggingManager.append_seq(alert.minute);
  loggingManager.append_seq(" ");
  loggingManager.append_seq(this->int_to_weekday(alert.day));
  loggingManager.end_seq();
}

bool AlertManager::timer_is_active_on_weekday(timer_config_t timer, int weekday)
{
  if (timer.enabled)
  {
    if (timer.monday && weekday == 1)
    {
      return true;
    }
    else if (timer.tuesday && weekday == 2)
    {
      return true;
    }
    else if (timer.wednesday && weekday == 3)
    {
      return true;
    }
    else if (timer.thursday && weekday == 4)
    {
      return true;
    }
    else if (timer.friday && weekday == 5)
    {
      return true;
    }
    else if (timer.saturday && weekday == 6)
    {
      return true;
    }
    else if (timer.sunday && weekday == 7)
    {
      return true;
    }
  }

  return false;
}

// debugging functions
void AlertManager::print_now()
{
  ds3231_datetime_t now = this->now();

  // Display the date
  loggingManager.start_seq(LOG_LEVEL_INFO, "date: ");
  loggingManager.append_seq(now.year);
  loggingManager.append_seq("-");
  loggingManager.append_seq(now.month);
  loggingManager.append_seq("-");
  loggingManager.append_seq(now.day);
  loggingManager.append_seq(" ");

  // Display the time
  loggingManager.append_seq(now.hour);
  loggingManager.append_seq(":");
  loggingManager.append_seq(now.minute);
  loggingManager.append_seq(":");
  loggingManager.append_seq(now.second);
  loggingManager.append_seq(" ");
  loggingManager.end_seq();
}

void AlertManager::print_temperature()
{
  loggingManager.start_seq(LOG_LEVEL_INFO, "temperature: ");
  loggingManager.append_seq(this->now().temperature);
  loggingManager.append_seq("°C");
  loggingManager.end_seq();
}

void AlertManager::print_timer(ds3231_timer_t timer)
{
  loggingManager.start_seq(LOG_LEVEL_INFO, "timer: ");
  loggingManager.append_seq(timer.hour);
  loggingManager.append_seq(":");
  loggingManager.append_seq(timer.minute);
  loggingManager.append_seq(" ");
  loggingManager.append_seq(this->int_to_weekday(timer.weekday));
  loggingManager.end_seq();
}

void AlertManager::print_timer(timer_config_t timer)
{
  loggingManager.start_seq(LOG_LEVEL_INFO, "timer: ");
  loggingManager.append_seq(timer.time.hour);
  loggingManager.append_seq(":");
  loggingManager.append_seq(timer.time.minute);
  loggingManager.append_seq(" ");
  loggingManager.append_seq(this->int_to_weekday(this->get_next_weekday_from_timer(timer, this->now().weekday)));
  loggingManager.end_seq();
}

void AlertManager::print_timer_list(ds3231_timer_list_t timers)
{
  loggingManager.start_seq(LOG_LEVEL_INFO, "timers: ");
  for (size_t i = 0; i < timers.num_timers; i++)
  {
    ds3231_timer_t timer = timers.timers[i];

    loggingManager.append_seq(timer.hour);
    loggingManager.append_seq(":");
    loggingManager.append_seq(timer.minute);
    loggingManager.append_seq(" ");
    loggingManager.append_seq(this->int_to_weekday(timer.weekday));

    if (i < timers.num_timers - 1)
    {
      loggingManager.append_seq(", ");
    }
  }
  loggingManager.end_seq();
}