#include "clock_service.h"
#include "rtc_driver.h"
#include "scheduler.h"
#include "time_manager.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "CLOCK_SERVICE";
static bool clock_service_initialized = false;
static clock_sleep_mode_t current_sleep_mode = CLOCK_SLEEP_MODE_DEEP;

esp_err_t clock_service_init(void) {
    ESP_LOGI(TAG, "Initializing clock service");
    
    if (clock_service_initialized) {
        ESP_LOGW(TAG, "Clock service already initialized");
        return ESP_OK;
    }
    
    // Initialize RTC driver
    esp_err_t ret = rtc_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RTC driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize scheduler
    ret = scheduler_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize scheduler: %s", esp_err_to_name(ret));
        rtc_driver_deinit();
        return ret;
    }
    
    // Check if RTC is running and has valid time
    if (!rtc_driver_is_running()) {
        ESP_LOGW(TAG, "RTC oscillator is not running - time may be invalid");
        
        // Set a default time if RTC is not running
        struct tm default_time = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 12,
            .tm_mday = 1,
            .tm_mon = 0,  // January
            .tm_year = 124,  // 2024
            .tm_wday = 1,  // Monday
        };
        
        ESP_LOGW(TAG, "Setting default time: 2024-01-01 12:00:00");
        rtc_driver_write_time(&default_time);
    }
    
    clock_service_initialized = true;
    
    // Log current time
    struct tm current_time;
    if (clock_service_get_time(&current_time) == ESP_OK) {
        char time_str[32];
        time_manager_format_datetime_string(&current_time, time_str, sizeof(time_str));
        ESP_LOGI(TAG, "Clock service initialized - Current time: %s", time_str);
    }
    
    return ESP_OK;
}

void clock_service_deinit(void) {
    if (clock_service_initialized) {
        scheduler_deinit();
        rtc_driver_deinit();
        clock_service_initialized = false;
        ESP_LOGI(TAG, "Clock service deinitialized");
    }
}

esp_err_t clock_service_get_time(struct tm* time_info) {
    if (!clock_service_initialized || !time_info) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return rtc_driver_read_time(time_info);
}

esp_err_t clock_service_set_time(struct tm* time_info) {
    if (!clock_service_initialized || !time_info) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Validate time
    if (!time_manager_is_valid_time(time_info)) {
        ESP_LOGE(TAG, "Invalid time provided");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = rtc_driver_write_time(time_info);
    if (ret == ESP_OK) {
        char time_str[32];
        time_manager_format_datetime_string(time_info, time_str, sizeof(time_str));
        ESP_LOGI(TAG, "Time updated: %s", time_str);
    }
    
    return ret;
}

bool clock_service_is_rtc_running(void) {
    if (!clock_service_initialized) {
        return false;
    }
    
    return rtc_driver_is_running();
}

esp_err_t clock_service_add_schedule(feeding_schedule_t* schedule) {
    if (!clock_service_initialized || !schedule) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return scheduler_add_entry(schedule);
}

esp_err_t clock_service_remove_schedule(uint8_t schedule_id) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return scheduler_remove_entry(schedule_id);
}

esp_err_t clock_service_get_schedules(feeding_schedule_t* schedules, uint8_t* count) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return scheduler_get_entries(schedules, count);
}

esp_err_t clock_service_clear_schedules(void) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return scheduler_clear_entries();
}

esp_err_t clock_service_enable_scheduler(bool enable) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return scheduler_set_enabled(enable);
}

bool clock_service_is_scheduler_enabled(void) {
    if (!clock_service_initialized) {
        return false;
    }
    
    return scheduler_is_enabled();
}

void clock_service_process(void) {
    if (!clock_service_initialized) {
        return;
    }
    
    // Get current time
    struct tm current_time;
    if (rtc_driver_read_time(&current_time) != ESP_OK) {
        return;
    }
    
    // Check if any feeding is due
    if (scheduler_check_feeding_time(&current_time)) {
        ESP_LOGI(TAG, "Feeding time detected!");
        // TODO: Trigger feeding mechanism
        // feeding_start(); // This would be called from main application
    }
}

esp_err_t clock_service_get_next_feeding_time(struct tm* next_time) {
    if (!clock_service_initialized || !next_time) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return scheduler_get_next_feeding_time(next_time);
}

esp_err_t clock_service_get_temperature(float* temperature) {
    if (!clock_service_initialized || !temperature) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return rtc_driver_read_temperature(temperature);
}

esp_err_t clock_service_format_time_string(struct tm* time_info, char* buffer, size_t buffer_size) {
    if (!clock_service_initialized || !time_info || !buffer) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return time_manager_format_datetime_string(time_info, buffer, buffer_size);
}

// Deep sleep and wake-up functions

esp_err_t clock_service_init_wake_interrupt(void) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Configure GPIO21 as input for DS3231 INT/SQW pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << CLOCK_SERVICE_INT_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 1,  // Internal pull-up (DS3231 INT is open-drain, active low)
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure interrupt GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure as RTC wake-up source (ESP32-C6 supports this)
    ret = esp_sleep_enable_ext1_wakeup((1ULL << CLOCK_SERVICE_INT_GPIO), ESP_EXT1_WAKEUP_ANY_LOW);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable ext1 wakeup: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Wake interrupt configured on GPIO%d", CLOCK_SERVICE_INT_GPIO);
    return ESP_OK;
}

esp_err_t clock_service_setup_wake_alarm(uint32_t seconds_from_now) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Get current time
    struct tm current_time;
    esp_err_t ret = rtc_driver_read_time(&current_time);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read current time for alarm setup: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Calculate alarm time
    time_t current_timestamp = mktime(&current_time);
    current_timestamp += seconds_from_now;
    struct tm* alarm_time = localtime(&current_timestamp);
    
    // Configure DS3231 Alarm 1 (more precise, includes seconds)
    alarm_config_t alarm = {
        .alarm_num = 1,
        .hour = alarm_time->tm_hour,
        .minute = alarm_time->tm_min,
        .second = alarm_time->tm_sec,
        .repeat_daily = false
    };
    
    ret = rtc_driver_set_alarm(&alarm);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wake alarm: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Enable alarm 1
    ret = rtc_driver_enable_alarm(1, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable wake alarm: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Wake alarm set for %d seconds from now (%02d:%02d:%02d)", 
             seconds_from_now, alarm_time->tm_hour, alarm_time->tm_min, alarm_time->tm_sec);
    
    return ESP_OK;
}

bool clock_service_is_wake_from_alarm(void) {
    if (!clock_service_initialized) {
        return false;
    }
    
    // Check ESP32 wake cause
    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
    if (wake_cause != ESP_SLEEP_WAKEUP_EXT1) {
        return false;
    }
    
    // Check if DS3231 alarm 1 was triggered
    bool alarm_triggered = rtc_driver_is_alarm_triggered(1);
    
    if (alarm_triggered) {
        ESP_LOGI(TAG, "System woke up from DS3231 alarm interrupt");
        return true;
    }
    
    return false;
}

void clock_service_clear_wake_alarm(void) {
    if (!clock_service_initialized) {
        return;
    }
    
    // Clear DS3231 alarm flag
    rtc_driver_clear_alarm(1);
    
    // Disable alarm 1
    rtc_driver_enable_alarm(1, false);
    
    ESP_LOGI(TAG, "Wake alarm cleared and disabled");
}

esp_err_t clock_service_set_sleep_mode(clock_sleep_mode_t mode) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    current_sleep_mode = mode;
    
    const char* mode_names[] = {"DEEP", "LIGHT", "MODEM"};
    ESP_LOGI(TAG, "Sleep mode set to: %s", mode_names[mode]);
    
    return ESP_OK;
}

esp_err_t clock_service_enter_light_sleep(uint32_t seconds) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Entering light sleep for %lu seconds (WiFi preserved)...", seconds);
    
    // Configure wake-up sources for light sleep
    // 1. Timer wake-up
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL); // Convert to microseconds
    
    // 2. GPIO wake-up (DS3231 interrupt)
    esp_sleep_enable_ext1_wakeup((1ULL << CLOCK_SERVICE_INT_GPIO), ESP_EXT1_WAKEUP_ANY_LOW);
    
    // Small delay to ensure log messages are sent
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Enter light sleep - WiFi connection will be maintained
    esp_err_t ret = esp_light_sleep_start();
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Woke up from light sleep");
        
        // Check wake cause
        esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
        switch (wake_cause) {
            case ESP_SLEEP_WAKEUP_TIMER:
                ESP_LOGI(TAG, "Wake cause: Timer");
                break;
            case ESP_SLEEP_WAKEUP_EXT1:
                ESP_LOGI(TAG, "Wake cause: DS3231 alarm interrupt");
                break;
            default:
                ESP_LOGI(TAG, "Wake cause: Other (%d)", wake_cause);
                break;
        }
    } else {
        ESP_LOGE(TAG, "Light sleep failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t clock_service_enter_deep_sleep(void) {
    if (!clock_service_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Select sleep mode based on current setting
    switch (current_sleep_mode) {
        case CLOCK_SLEEP_MODE_LIGHT:
            ESP_LOGI(TAG, "Using light sleep mode (WiFi preserved)");
            return clock_service_enter_light_sleep(10); // Default 10 seconds
            
        case CLOCK_SLEEP_MODE_MODEM:
            ESP_LOGI(TAG, "Using modem sleep mode (staying awake with power save)");
            // Just return - stay awake but enable WiFi power saving
            return ESP_OK;
            
        case CLOCK_SLEEP_MODE_DEEP:
        default:
            ESP_LOGI(TAG, "Entering deep sleep mode...");
            ESP_LOGI(TAG, "Wake-up source: DS3231 alarm on GPIO%d", CLOCK_SERVICE_INT_GPIO);
            
            // Small delay to ensure log messages are sent
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Enter deep sleep - will wake up on GPIO0 going low (DS3231 alarm)
            esp_deep_sleep_start();
            
            // This line should never be reached
            return ESP_ERR_INVALID_STATE;
    }
}