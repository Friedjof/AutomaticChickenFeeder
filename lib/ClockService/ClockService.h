#pragma once

#include <RTClib.h>
#include <time.h>

#define CENTURY 2000

#if defined(ESP8266)
#define SDA_PIN 21 // D21
#define SCL_PIN 22 // D22
#elif defined(ESP32DEV)
#define SDA_PIN 21 // D21
#define SCL_PIN 22 // D22
#elif defined(ESP32S3)
#define SDA_PIN 4 // D4
#define SCL_PIN 5 // D5
#endif

class ClockService
{
private:
  RTC_DS3231 rtc;

  bool initialized = false;

public:
  ClockService();
  ~ClockService();

  void begin();
  bool is_initialized();

  void set_datetime(DateTime datetime);
  void set_datetime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second);
  DateTime get_datetime();

  uint16_t getYear();
  uint16_t getMonth();
  uint16_t getDay();
  uint16_t getHour();
  uint16_t getMinute();
  uint16_t getSecond();
  uint16_t getDoW();
  uint16_t getTemperature();

  void clear_alerts();
  String getDoWString();

  void turnOffAlarm(byte alarm);
  void disableAlarm2();

  void setA1Time(DateTime time);

  double getTemperatureInCelsius();
  double getTemperatureInFahrenheit();

  String datetime_as_string();
};