#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Time validation and utilities
bool time_manager_is_valid_time(struct tm* time_info);
bool time_manager_is_leap_year(int year);
uint8_t time_manager_days_in_month(int month, int year);

// Time calculations
uint32_t time_manager_seconds_until(struct tm* target_time);
int time_manager_compare_times(struct tm* time1, struct tm* time2);
esp_err_t time_manager_add_minutes(struct tm* time_info, int minutes);

// Time formatting
esp_err_t time_manager_format_time_string(struct tm* time_info, char* buffer, size_t buffer_size);
esp_err_t time_manager_format_date_string(struct tm* time_info, char* buffer, size_t buffer_size);
esp_err_t time_manager_format_datetime_string(struct tm* time_info, char* buffer, size_t buffer_size);

// Weekday utilities
const char* time_manager_weekday_name(int weekday);
const char* time_manager_weekday_short_name(int weekday);
const char* time_manager_month_name(int month);
const char* time_manager_month_short_name(int month);

// Time zone utilities (for future use)
esp_err_t time_manager_set_timezone(const char* timezone);
esp_err_t time_manager_get_timezone(char* timezone, size_t size);

#ifdef __cplusplus
}
#endif

#endif // TIME_MANAGER_H