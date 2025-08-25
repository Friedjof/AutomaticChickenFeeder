#ifndef CLOCK_SERVICE_H
#define CLOCK_SERVICE_H

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration
#define CLOCK_SERVICE_MAX_SCHEDULES 16
#define CLOCK_SERVICE_SCHEDULE_NAME_LEN 32

// DS3231 I2C pins
#define CLOCK_SERVICE_SDA_GPIO 22  // D4 on Seeed XIAO ESP32-C6
#define CLOCK_SERVICE_SCL_GPIO 23  // D5 on Seeed XIAO ESP32-C6
#define CLOCK_SERVICE_INT_GPIO 0   // GPIO0 - RTC_GPIO on ESP32-C6 for wake-up

// Weekday bitmask definitions
#define CLOCK_SUNDAY    (1 << 0)
#define CLOCK_MONDAY    (1 << 1)
#define CLOCK_TUESDAY   (1 << 2)
#define CLOCK_WEDNESDAY (1 << 3)
#define CLOCK_THURSDAY  (1 << 4)
#define CLOCK_FRIDAY    (1 << 5)
#define CLOCK_SATURDAY  (1 << 6)
#define CLOCK_WEEKDAYS  (0x3E)  // Mon-Fri
#define CLOCK_WEEKEND   (0x41)  // Sat-Sun
#define CLOCK_DAILY     (0x7F)  // All days

// Feeding schedule structure
typedef struct {
    uint8_t id;                                            // Unique schedule ID
    uint8_t hour;                                          // Hour (0-23)
    uint8_t minute;                                        // Minute (0-59)
    uint8_t weekdays;                                      // Weekday bitmask
    bool enabled;                                          // Schedule active flag
    char name[CLOCK_SERVICE_SCHEDULE_NAME_LEN];            // Human-readable name
} feeding_schedule_t;

// Clock service initialization and cleanup
esp_err_t clock_service_init(void);
void clock_service_deinit(void);

// Time management
esp_err_t clock_service_get_time(struct tm* time_info);
esp_err_t clock_service_set_time(struct tm* time_info);
bool clock_service_is_rtc_running(void);

// Schedule management
esp_err_t clock_service_add_schedule(feeding_schedule_t* schedule);
esp_err_t clock_service_remove_schedule(uint8_t schedule_id);
esp_err_t clock_service_get_schedules(feeding_schedule_t* schedules, uint8_t* count);
esp_err_t clock_service_clear_schedules(void);

// Schedule control
esp_err_t clock_service_enable_scheduler(bool enable);
bool clock_service_is_scheduler_enabled(void);

// Runtime processing
void clock_service_process(void);
esp_err_t clock_service_get_next_feeding_time(struct tm* next_time);

// Temperature reading from DS3231
esp_err_t clock_service_get_temperature(float* temperature);

// Utility functions
esp_err_t clock_service_format_time_string(struct tm* time_info, char* buffer, size_t buffer_size);

// Deep sleep and interrupt functions
esp_err_t clock_service_setup_wake_alarm(uint32_t seconds_from_now);
esp_err_t clock_service_enter_deep_sleep(void);
esp_err_t clock_service_enter_light_sleep(uint32_t seconds);  // NEW: WiFi-preserving sleep
esp_err_t clock_service_init_wake_interrupt(void);
bool clock_service_is_wake_from_alarm(void);
void clock_service_clear_wake_alarm(void);

// Sleep mode selection
typedef enum {
    CLOCK_SLEEP_MODE_DEEP,      // Ultra-low power, no WiFi/ZigBee
    CLOCK_SLEEP_MODE_LIGHT,     // Low power, WiFi preserved  
    CLOCK_SLEEP_MODE_MODEM      // Minimal power save, full connectivity
} clock_sleep_mode_t;

esp_err_t clock_service_set_sleep_mode(clock_sleep_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // CLOCK_SERVICE_H