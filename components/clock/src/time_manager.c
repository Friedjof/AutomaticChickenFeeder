#include "time_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "TIME_MANAGER";

// Day names
static const char* weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char* weekday_short_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

// Month names
static const char* month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const char* month_short_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// Days in each month (non-leap year)
static const uint8_t days_in_month_table[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

bool time_manager_is_valid_time(struct tm* time_info) {
    if (!time_info) return false;
    
    // Check basic ranges
    if (time_info->tm_sec < 0 || time_info->tm_sec > 59) return false;
    if (time_info->tm_min < 0 || time_info->tm_min > 59) return false;
    if (time_info->tm_hour < 0 || time_info->tm_hour > 23) return false;
    if (time_info->tm_mday < 1 || time_info->tm_mday > 31) return false;
    if (time_info->tm_mon < 0 || time_info->tm_mon > 11) return false;
    if (time_info->tm_year < 0) return false;  // Years since 1900
    if (time_info->tm_wday < 0 || time_info->tm_wday > 6) return false;
    
    // Check days in month
    int year = time_info->tm_year + 1900;
    uint8_t max_days = time_manager_days_in_month(time_info->tm_mon, year);
    if (time_info->tm_mday > max_days) return false;
    
    return true;
}

bool time_manager_is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t time_manager_days_in_month(int month, int year) {
    if (month < 0 || month > 11) return 0;
    
    uint8_t days = days_in_month_table[month];
    
    // Handle February in leap years
    if (month == 1 && time_manager_is_leap_year(year)) {
        days = 29;
    }
    
    return days;
}

uint32_t time_manager_seconds_until(struct tm* target_time) {
    if (!target_time) return 0;
    
    time_t current_time = time(NULL);
    time_t target_timestamp = mktime(target_time);
    
    if (target_timestamp <= current_time) return 0;
    
    return (uint32_t)(target_timestamp - current_time);
}

int time_manager_compare_times(struct tm* time1, struct tm* time2) {
    if (!time1 || !time2) return 0;
    
    time_t timestamp1 = mktime(time1);
    time_t timestamp2 = mktime(time2);
    
    if (timestamp1 < timestamp2) return -1;
    if (timestamp1 > timestamp2) return 1;
    return 0;
}

esp_err_t time_manager_add_minutes(struct tm* time_info, int minutes) {
    if (!time_info) return ESP_ERR_INVALID_ARG;
    
    time_t timestamp = mktime(time_info);
    timestamp += minutes * 60;
    
    struct tm* new_time = localtime(&timestamp);
    if (!new_time) return ESP_ERR_INVALID_STATE;
    
    memcpy(time_info, new_time, sizeof(struct tm));
    return ESP_OK;
}

esp_err_t time_manager_format_time_string(struct tm* time_info, char* buffer, size_t buffer_size) {
    if (!time_info || !buffer || buffer_size < 9) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int ret = snprintf(buffer, buffer_size, "%02d:%02d:%02d", 
                       time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    
    if (ret < 0 || ret >= buffer_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    return ESP_OK;
}

esp_err_t time_manager_format_date_string(struct tm* time_info, char* buffer, size_t buffer_size) {
    if (!time_info || !buffer || buffer_size < 11) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int ret = snprintf(buffer, buffer_size, "%04d-%02d-%02d", 
                       time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday);
    
    if (ret < 0 || ret >= buffer_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    return ESP_OK;
}

esp_err_t time_manager_format_datetime_string(struct tm* time_info, char* buffer, size_t buffer_size) {
    if (!time_info || !buffer || buffer_size < 20) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int ret = snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d", 
                       time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
                       time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    
    if (ret < 0 || ret >= buffer_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    return ESP_OK;
}

const char* time_manager_weekday_name(int weekday) {
    if (weekday < 0 || weekday > 6) return "Unknown";
    return weekday_names[weekday];
}

const char* time_manager_weekday_short_name(int weekday) {
    if (weekday < 0 || weekday > 6) return "???";
    return weekday_short_names[weekday];
}

const char* time_manager_month_name(int month) {
    if (month < 0 || month > 11) return "Unknown";
    return month_names[month];
}

const char* time_manager_month_short_name(int month) {
    if (month < 0 || month > 11) return "???";
    return month_short_names[month];
}

esp_err_t time_manager_set_timezone(const char* timezone) {
    if (!timezone) return ESP_ERR_INVALID_ARG;
    
    // Set timezone environment variable
    setenv("TZ", timezone, 1);
    tzset();
    
    ESP_LOGI(TAG, "Timezone set to: %s", timezone);
    return ESP_OK;
}

esp_err_t time_manager_get_timezone(char* timezone, size_t size) {
    if (!timezone || size == 0) return ESP_ERR_INVALID_ARG;
    
    const char* tz = getenv("TZ");
    if (!tz) {
        strncpy(timezone, "UTC", size - 1);
        timezone[size - 1] = '\0';
    } else {
        strncpy(timezone, tz, size - 1);
        timezone[size - 1] = '\0';
    }
    
    return ESP_OK;
}