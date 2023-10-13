#include "ClockService.h"

ClockService::ClockService(bool century, bool h12, bool pm) {
    this->century = century;
    this->h12Flag = h12;
    this->pmFlag = pm;

    // Setup ClockService
    this->setup();
}

ClockService::ClockService() {
    this->century = false;
    this->h12Flag = false;
    this->pmFlag = false;
    
    // Setup ClockService
    this->setup();
}

ClockService::~ClockService() {
    delete this->rtc;
}

void ClockService::setup() {
    Wire.begin();

    this->rtc->setClockMode(this->h12Flag);
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
    return this->rtc->getYear();
}

uint16_t ClockService::getMonth() {
    return this->rtc->getMonth(this->century);
}

uint16_t ClockService::getDay() {
    return this->rtc->getDate();
}

uint16_t ClockService::getHour() {
    return this->rtc->getHour(this->h12Flag, this->pmFlag);
}

uint16_t ClockService::getMinute() {
    return this->rtc->getMinute();
}

uint16_t ClockService::getSecond() {
    return this->rtc->getSecond();
}

uint16_t ClockService::getDoW() {
    return this->rtc->getDoW();
}

uint16_t ClockService::getTemperature() {
    return this->rtc->getTemperature();
}

void ClockService::turnOffAlarm(byte alarm) {
    this->rtc->turnOffAlarm(alarm);
}

void ClockService::turnOnAlarm(byte alarm) {
    this->rtc->turnOnAlarm(alarm);
}

void ClockService::setA1Time(byte day, byte hour, byte minute, byte second, byte alarmBits, bool day_is_day) {
    this->rtc->setA1Time(day, hour, minute, second, alarmBits, day_is_day, this->h12Flag, this->pmFlag);
}

bool ClockService::checkIfAlarm(byte alarm) {
    return this->rtc->checkIfAlarm(alarm);
}

void ClockService::disableAlarm2() {
    this->rtc->setA2Time(0x00, 0x00, 0xff, 0x60, false, false, false); // 0x60 = 0b01100000 => Alarm when minutes match (never because of 0xff)
    this->rtc->turnOffAlarm(2);
    this->rtc->checkIfAlarm(2);
}

std::string ClockService::datetime_as_string() {
    std::string datetime_string = "";

    datetime_string += std::to_string(this->getYear());
    datetime_string += "-";
    datetime_string += std::to_string(this->getMonth());
    datetime_string += "-";
    datetime_string += std::to_string(this->getDay());
    datetime_string += " ";
    datetime_string += std::to_string(this->getHour());
    datetime_string += ":";
    datetime_string += std::to_string(this->getMinute());
    datetime_string += ":";
    datetime_string += std::to_string(this->getSecond());

    return datetime_string;
}