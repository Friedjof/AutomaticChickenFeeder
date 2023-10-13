#ifndef CLOCK_SERVICE_H
#define CLOCK_SERVICE_H

#include <Arduino.h>

#include <Wire.h>
#include <DS3231.h>
#include <time.h>


class ClockService {
    private:
        DS3231* rtc;
        bool century = false;
        bool h12Flag;
        bool pmFlag;

        void setup();

    public:
        ClockService(bool century, bool h12, bool pm);
        ClockService();
        ~ClockService();

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

        std::string datetime_as_string();
};

#endif