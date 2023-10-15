#pragma once

#include <DS3231.h>
#include <time.h>

#define CENTURY 2000

#define SDA_PIN 21   // D21
#define SCL_PIN 22   // D22


class ClockService {
  private:
    DS3231 rtc;
    bool century = false;
    bool h12Flag;
    bool pmFlag;

    bool initialized = false;
  public:
    ClockService(bool century, bool h12, bool pm);
    ClockService();
    ~ClockService();

    void begin();
    bool is_initialized();
    
    DateTime get_datetime();

    void set_century(bool century);
    bool get_century();

    void set_h12(bool h12);
    bool get_h12();

    void set_pm(bool pm);
    bool get_pm();

    uint16_t getYear();
    uint16_t getMonth();
    uint16_t getDay();
    uint16_t getHour();
    uint16_t getMinute();
    uint16_t getSecond();
    uint16_t getDoW();
    uint16_t getTemperature();

    void turnOffAlarm(byte alarm);
    void turnOnAlarm(byte alarm);
    
    void setA1Time(byte day, byte hour, byte minute, byte second, byte alarmBits, bool day_is_day);
    bool checkIfAlarm(byte alarm);

    void disableAlarm2();

    String datetime_as_string();
};