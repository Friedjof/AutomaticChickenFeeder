#include "scheduler.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "SCHEDULER";
static const char* NVS_NAMESPACE = "clock_sched";
static const char* NVS_KEY_SCHEDULES = "schedules";
static const char* NVS_KEY_ENABLED = "enabled";

static feeding_schedule_t schedules[CLOCK_SERVICE_MAX_SCHEDULES];
static uint8_t schedule_count = 0;
static bool scheduler_enabled = true;
static bool scheduler_initialized = false;

esp_err_t scheduler_init(void) {
    ESP_LOGI(TAG, "Initializing scheduler");
    
    // Initialize NVS if not already done
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs to be erased");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS flash: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS flash: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Load schedules from NVS
    ret = scheduler_load_from_nvs();
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to load schedules from NVS: %s", esp_err_to_name(ret));
        // Continue with empty schedule list
    }
    
    scheduler_initialized = true;
    ESP_LOGI(TAG, "Scheduler initialized with %d schedules", schedule_count);
    return ESP_OK;
}

void scheduler_deinit(void) {
    if (scheduler_initialized) {
        scheduler_save_to_nvs();
        scheduler_initialized = false;
        ESP_LOGI(TAG, "Scheduler deinitialized");
    }
}

esp_err_t scheduler_add_entry(feeding_schedule_t* schedule) {
    if (!scheduler_initialized || !schedule) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (schedule_count >= CLOCK_SERVICE_MAX_SCHEDULES) {
        ESP_LOGE(TAG, "Maximum number of schedules reached (%d)", CLOCK_SERVICE_MAX_SCHEDULES);
        return ESP_ERR_NO_MEM;
    }
    
    // Validate schedule parameters
    if (schedule->hour > 23 || schedule->minute > 59 || schedule->weekdays == 0) {
        ESP_LOGE(TAG, "Invalid schedule parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check for duplicate ID
    for (int i = 0; i < schedule_count; i++) {
        if (schedules[i].id == schedule->id) {
            ESP_LOGE(TAG, "Schedule ID %d already exists", schedule->id);
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // If ID is 0, auto-assign
    if (schedule->id == 0) {
        schedule->id = scheduler_get_next_schedule_id();
    }
    
    // Add schedule
    memcpy(&schedules[schedule_count], schedule, sizeof(feeding_schedule_t));
    schedule_count++;
    
    // Save to NVS
    esp_err_t ret = scheduler_save_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save schedules to NVS: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "Added schedule ID %d: %s at %02d:%02d", 
             schedule->id, schedule->name, schedule->hour, schedule->minute);
    return ESP_OK;
}

esp_err_t scheduler_remove_entry(uint8_t schedule_id) {
    if (!scheduler_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    for (int i = 0; i < schedule_count; i++) {
        if (schedules[i].id == schedule_id) {
            // Shift remaining schedules down
            for (int j = i; j < schedule_count - 1; j++) {
                memcpy(&schedules[j], &schedules[j + 1], sizeof(feeding_schedule_t));
            }
            schedule_count--;
            
            // Save to NVS
            esp_err_t ret = scheduler_save_to_nvs();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to save schedules to NVS: %s", esp_err_to_name(ret));
            }
            
            ESP_LOGI(TAG, "Removed schedule ID %d", schedule_id);
            return ESP_OK;
        }
    }
    
    ESP_LOGE(TAG, "Schedule ID %d not found", schedule_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t scheduler_get_entries(feeding_schedule_t* schedules_out, uint8_t* count) {
    if (!scheduler_initialized || !schedules_out || !count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(schedules_out, schedules, schedule_count * sizeof(feeding_schedule_t));
    *count = schedule_count;
    return ESP_OK;
}

esp_err_t scheduler_clear_entries(void) {
    if (!scheduler_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    schedule_count = 0;
    memset(schedules, 0, sizeof(schedules));
    
    esp_err_t ret = scheduler_save_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save cleared schedules to NVS: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "All schedules cleared");
    return ESP_OK;
}

bool scheduler_check_feeding_time(struct tm* current_time) {
    if (!scheduler_initialized || !scheduler_enabled || !current_time) {
        return false;
    }
    
    for (int i = 0; i < schedule_count; i++) {
        if (schedules[i].enabled && scheduler_is_schedule_due(&schedules[i], current_time)) {
            ESP_LOGI(TAG, "Feeding time reached for schedule: %s", schedules[i].name);
            return true;
        }
    }
    
    return false;
}

esp_err_t scheduler_get_next_feeding_time(struct tm* next_time) {
    if (!scheduler_initialized || !next_time) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t current_timestamp = time(NULL);
    time_t next_timestamp = 0;
    bool found = false;
    
    // Check each enabled schedule
    for (int i = 0; i < schedule_count; i++) {
        if (!schedules[i].enabled) continue;
        
        // Calculate next occurrence for this schedule
        for (int days_ahead = 0; days_ahead < 8; days_ahead++) {  // Check up to 1 week ahead
            time_t check_timestamp = current_timestamp + (days_ahead * 24 * 60 * 60);
            struct tm check_time;
            localtime_r(&check_timestamp, &check_time);
            
            // Check if this weekday is enabled for the schedule
            uint8_t weekday_bit = 1 << check_time.tm_wday;
            if (!(schedules[i].weekdays & weekday_bit)) continue;
            
            // Set time to schedule time
            check_time.tm_hour = schedules[i].hour;
            check_time.tm_min = schedules[i].minute;
            check_time.tm_sec = 0;
            
            time_t schedule_timestamp = mktime(&check_time);
            
            // Skip if this time has already passed today
            if (schedule_timestamp <= current_timestamp) continue;
            
            // Check if this is the earliest next feeding time
            if (!found || schedule_timestamp < next_timestamp) {
                next_timestamp = schedule_timestamp;
                found = true;
            }
            break;  // Found next occurrence for this schedule
        }
    }
    
    if (found) {
        localtime_r(&next_timestamp, next_time);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "No upcoming feeding times found");
        return ESP_ERR_NOT_FOUND;
    }
}

esp_err_t scheduler_set_enabled(bool enabled) {
    if (!scheduler_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    scheduler_enabled = enabled;
    
    // Save enabled state to NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_KEY_ENABLED, enabled ? 1 : 0);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    
    ESP_LOGI(TAG, "Scheduler %s", enabled ? "enabled" : "disabled");
    return ESP_OK;
}

bool scheduler_is_enabled(void) {
    return scheduler_initialized && scheduler_enabled;
}

bool scheduler_is_schedule_due(feeding_schedule_t* schedule, struct tm* current_time) {
    if (!schedule || !current_time) return false;
    
    // Check if current weekday is enabled
    uint8_t current_weekday_bit = 1 << current_time->tm_wday;
    if (!(schedule->weekdays & current_weekday_bit)) return false;
    
    // Check if current time matches schedule time (within 1 minute window)
    if (current_time->tm_hour == schedule->hour && current_time->tm_min == schedule->minute) {
        return true;
    }
    
    return false;
}

uint8_t scheduler_get_next_schedule_id(void) {
    uint8_t max_id = 0;
    for (int i = 0; i < schedule_count; i++) {
        if (schedules[i].id > max_id) {
            max_id = schedules[i].id;
        }
    }
    return max_id + 1;
}

esp_err_t scheduler_save_to_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Save schedules
    size_t schedules_size = schedule_count * sizeof(feeding_schedule_t);
    ret = nvs_set_blob(nvs_handle, NVS_KEY_SCHEDULES, schedules, schedules_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save schedules to NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    // Save schedule count
    ret = nvs_set_u8(nvs_handle, "count", schedule_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save schedule count to NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Saved %d schedules to NVS", schedule_count);
    }
    
    return ret;
}

esp_err_t scheduler_load_from_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved schedules found in NVS");
            return ESP_ERR_NVS_NOT_FOUND;
        }
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Load schedule count
    ret = nvs_get_u8(nvs_handle, "count", &schedule_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load schedule count from NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    if (schedule_count > CLOCK_SERVICE_MAX_SCHEDULES) {
        ESP_LOGE(TAG, "Invalid schedule count in NVS: %d", schedule_count);
        schedule_count = 0;
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Load schedules
    if (schedule_count > 0) {
        size_t schedules_size = schedule_count * sizeof(feeding_schedule_t);
        ret = nvs_get_blob(nvs_handle, NVS_KEY_SCHEDULES, schedules, &schedules_size);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load schedules from NVS: %s", esp_err_to_name(ret));
            schedule_count = 0;
            nvs_close(nvs_handle);
            return ret;
        }
    }
    
    // Load enabled state
    uint8_t enabled_byte = 1;
    nvs_get_u8(nvs_handle, NVS_KEY_ENABLED, &enabled_byte);
    scheduler_enabled = (enabled_byte != 0);
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Loaded %d schedules from NVS", schedule_count);
    return ESP_OK;
}