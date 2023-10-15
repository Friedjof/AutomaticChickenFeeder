#include "ClockService.h"

ClockService::ClockService(bool century, bool h12, bool pm) {
  this->century = century;
  this->h12Flag = h12;
  this->pmFlag = pm;
}

ClockService::ClockService() {
  this->century = false;
  this->h12Flag = false;
  this->pmFlag = false;
}

ClockService::~ClockService() {
  // Nothing to do here
}

void ClockService::begin() {
  if (this->initialized) {
    return;
  }

  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  delay(1000);

  this->rtc.setClockMode(this->h12Flag);

  this->initialized = true;
}

bool ClockService::is_initialized() {
  return this->initialized;
}

DateTime ClockService::get_datetime() {
  DateTime datetime(
    this->getYear(), this->getMonth(), this->getDay(),
    this->getHour(), this->getMinute(), this->getSecond()
  );

  return datetime;
}

void ClockService::set_century(bool century) {
  this->century = century;
}

bool ClockService::get_century() {
  return this->century;
}

void ClockService::set_h12(bool h12) {
  this->h12Flag = h12;
}

bool ClockService::get_h12() {
  return this->h12Flag;
}

void ClockService::set_pm(bool pm) {
  this->pmFlag = pm;
}

bool ClockService::get_pm() {
  return this->pmFlag;
}

uint16_t ClockService::getYear() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getYear();
}

uint16_t ClockService::getMonth() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getMonth(this->century);
}

uint16_t ClockService::getDay() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getDate();
}

uint16_t ClockService::getHour() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getHour(this->h12Flag, this->pmFlag);
}

uint16_t ClockService::getMinute() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getMinute();
}

uint16_t ClockService::getSecond() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getSecond();
}

uint16_t ClockService::getDoW() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getDoW();
}

uint16_t ClockService::getTemperature() {
  if (!this->initialized) {
    return 0;
  }

  return this->rtc.getTemperature();
}

void ClockService::turnOffAlarm(byte alarm) {
  if (!this->initialized) {
    return;
  }

  this->rtc.turnOffAlarm(alarm);
}

void ClockService::turnOnAlarm(byte alarm) {
  if (!this->initialized) {
    return;
  }

  this->rtc.turnOnAlarm(alarm);
}

void ClockService::setA1Time(byte day, byte hour, byte minute, byte second, byte alarmBits, bool day_is_day) {
  if (!this->initialized) {
    return;
  }

  this->rtc.setA1Time(day, hour, minute, second, alarmBits, day_is_day, this->h12Flag, this->pmFlag);
}

bool ClockService::checkIfAlarm(byte alarm) {
  if (!this->initialized) {
    return false;
  }

  return this->rtc.checkIfAlarm(alarm);
}

void ClockService::disableAlarm2() {
  if (!this->initialized) {
    return;
  }

  this->rtc.setA2Time(0x00, 0x00, 0xff, 0x60, false, false, false); // 0x60 = 0b01100000 => Alarm when minutes match (never because of 0xff)
  this->rtc.turnOffAlarm(2);
  this->rtc.checkIfAlarm(2);
}

/*
  @Return datetime as string in format: YYYY-MM-DD HH:MM:SS
  Important: the year is 2000 + the actual year and every value should have 2 digits
*/
String ClockService::datetime_as_string() {
  String yyyy = String(CENTURY + this->getYear());
  String mm = String(this->getMonth());
  String dd = String(this->getDay());
  String hh = String(this->getHour());
  String ii = String(this->getMinute());
  String ss = String(this->getSecond());

  mm = mm.length() == 1 ? "0" + mm : mm;
  dd = dd.length() == 1 ? "0" + dd : dd;
  hh = hh.length() == 1 ? "0" + hh : hh;
  ii = ii.length() == 1 ? "0" + ii : ii;
  ss = ss.length() == 1 ? "0" + ss : ss;

  return yyyy + "-" + mm + "-" + dd + " " + hh + ":" + ii + ":" + ss;
}