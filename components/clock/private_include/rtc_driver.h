#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// DS3231 I2C address
#define DS3231_I2C_ADDR 0x68

// DS3231 register addresses
#define DS3231_REG_SECONDS     0x00
#define DS3231_REG_MINUTES     0x01
#define DS3231_REG_HOURS       0x02
#define DS3231_REG_DAY         0x03
#define DS3231_REG_DATE        0x04
#define DS3231_REG_MONTH       0x05
#define DS3231_REG_YEAR        0x06
#define DS3231_REG_ALARM1_SEC  0x07
#define DS3231_REG_ALARM1_MIN  0x08
#define DS3231_REG_ALARM1_HOUR 0x09
#define DS3231_REG_ALARM1_DATE 0x0A
#define DS3231_REG_ALARM2_MIN  0x0B
#define DS3231_REG_ALARM2_HOUR 0x0C
#define DS3231_REG_ALARM2_DATE 0x0D
#define DS3231_REG_CONTROL     0x0E
#define DS3231_REG_STATUS      0x0F
#define DS3231_REG_TEMP_MSB    0x11
#define DS3231_REG_TEMP_LSB    0x12

// Control register bits
#define DS3231_CTRL_A1IE  (1 << 0)  // Alarm 1 Interrupt Enable
#define DS3231_CTRL_A2IE  (1 << 1)  // Alarm 2 Interrupt Enable
#define DS3231_CTRL_INTCN (1 << 2)  // Interrupt Control
#define DS3231_CTRL_RS1   (1 << 3)  // Rate Select 1
#define DS3231_CTRL_RS2   (1 << 4)  // Rate Select 2
#define DS3231_CTRL_CONV  (1 << 5)  // Convert Temperature
#define DS3231_CTRL_BBSQW (1 << 6)  // Battery-Backed Square Wave
#define DS3231_CTRL_EOSC  (1 << 7)  // Enable Oscillator

// Status register bits
#define DS3231_STAT_A1F   (1 << 0)  // Alarm 1 Flag
#define DS3231_STAT_A2F   (1 << 1)  // Alarm 2 Flag
#define DS3231_STAT_BSY   (1 << 2)  // Busy
#define DS3231_STAT_EN32K (1 << 3)  // Enable 32kHz
#define DS3231_STAT_OSF   (1 << 7)  // Oscillator Stop Flag

// Alarm configuration
typedef struct {
    uint8_t alarm_num;      // 1 or 2
    uint8_t hour;           // 0-23
    uint8_t minute;         // 0-59
    uint8_t second;         // 0-59 (Alarm 1 only)
    bool repeat_daily;      // Repeat every day
} alarm_config_t;

// RTC Driver functions
esp_err_t rtc_driver_init(void);
void rtc_driver_deinit(void);

// Time operations
esp_err_t rtc_driver_read_time(struct tm* time_info);
esp_err_t rtc_driver_write_time(struct tm* time_info);
bool rtc_driver_is_running(void);

// Alarm operations
esp_err_t rtc_driver_set_alarm(alarm_config_t* alarm);
esp_err_t rtc_driver_enable_alarm(uint8_t alarm_num, bool enable);
bool rtc_driver_is_alarm_triggered(uint8_t alarm_num);
esp_err_t rtc_driver_clear_alarm(uint8_t alarm_num);

// Temperature reading
esp_err_t rtc_driver_read_temperature(float* temperature);

// Internal helper functions
uint8_t bcd_to_dec(uint8_t bcd);
uint8_t dec_to_bcd(uint8_t dec);

#ifdef __cplusplus
}
#endif

#endif // RTC_DRIVER_H