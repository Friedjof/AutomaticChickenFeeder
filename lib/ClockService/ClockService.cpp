#include "ClockService.h"

ClockService::ClockService()
{
  // Nothing to do here
}

ClockService::~ClockService()
{
  // Nothing to do here
}

void ClockService::begin()
{
  if (this->initialized)
  {
    return;
  }

  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);

  delay(500);

  // Initialize RTC
  if (!rtc.begin())
  {
    Serial.printf("[ERROR] Couldn't find RTC. Make sure you have connected the RTC to the I2C pins: SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);
    while (1)
      ;
  }

  // Check if RTC lost power and if so, set the time
  if (rtc.lostPower())
  {
    Serial.println("[INFO] RTC lost power, you need to set the time!");
  }

  this->rtc.writeSqwPinMode(DS3231_OFF);
  this->rtc.disable32K();
  
  this->rtc.disableAlarm(1);
  this->rtc.disableAlarm(2);

  this->initialized = true;
}

bool ClockService::is_initialized()
{
  return this->initialized;
}

void ClockService::set_datetime(DateTime datetime)
{
  if (!this->initialized)
  {
    return;
  }

  this->rtc.adjust(datetime);
}

void ClockService::set_datetime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second)
{
  if (!this->initialized)
  {
    return;
  }

  DateTime datetime(year, month, day, hour, minute, second);
  this->rtc.adjust(datetime);
}

DateTime ClockService::get_datetime()
{
  if (!this->initialized)
  {
    return DateTime();
  }

  return this->rtc.now();
}

uint16_t ClockService::getYear()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.year();
}

uint16_t ClockService::getMonth()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.month();
}

uint16_t ClockService::getDay()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.day();
}

uint16_t ClockService::getHour()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.hour();
}

uint16_t ClockService::getMinute()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.minute();
}

uint16_t ClockService::getSecond()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.second();
}

uint16_t ClockService::getDoW()
{
  if (!this->initialized)
  {
    return 0;
  }

  DateTime now = this->rtc.now();
  return now.dayOfTheWeek();
}

String ClockService::getDoWString()
{
  if (!this->initialized)
  {
    return "";
  }

  String daysOfTheWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  return daysOfTheWeek[this->getDoW() - 1];
}

uint16_t ClockService::getTemperature()
{
  if (!this->initialized)
  {
    return 0;
  }

  return this->rtc.getTemperature();
}

void ClockService::turnOffAlarm(byte alarm)
{
  if (!this->initialized)
  {
    return;
  }

  this->rtc.disableAlarm((uint8_t)alarm);
}

void ClockService::clear_alerts()
{
  if (!this->initialized)
  {
    return;
  }

  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF);
}

void ClockService::setA1Time(DateTime datetime)
{
  if (!this->initialized)
  {
    return;
  }

  Serial.println("Setting alarm 1");
  Serial.print("Alarm 1 time: ");
  Serial.println(datetime.timestamp(DateTime::TIMESTAMP_FULL));

  this->clear_alerts();

  this->rtc.setAlarm1(datetime, DS3231_A1_Date);

  Serial.println("Alarm 1 set");
}

void ClockService::disableAlarm2()
{
  if (!this->initialized)
  {
    return;
  }

  this->rtc.disableAlarm(2);
}

double ClockService::getTemperatureInCelsius()
{
  if (!this->initialized)
  {
    return 0;
  }

  return this->rtc.getTemperature();
}

double ClockService::getTemperatureInFahrenheit()
{
  if (!this->initialized)
  {
    return 0;
  }

  return this->rtc.getTemperature() * 1.8 + 32;
}

String ClockService::datetime_as_string()
{
  if (!this->initialized)
  {
    return "";
  }

  return this->rtc.now().timestamp(DateTime::TIMESTAMP_FULL);
}