#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "clock_service.h"
#include "esp_err.h"
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scheduler internal functions
esp_err_t scheduler_init(void);
void scheduler_deinit(void);

// Schedule management
esp_err_t scheduler_add_entry(feeding_schedule_t* schedule);
esp_err_t scheduler_remove_entry(uint8_t schedule_id);
esp_err_t scheduler_get_entries(feeding_schedule_t* schedules, uint8_t* count);
esp_err_t scheduler_clear_entries(void);

// Schedule processing
bool scheduler_check_feeding_time(struct tm* current_time);
esp_err_t scheduler_get_next_feeding_time(struct tm* next_time);

// Scheduler control
esp_err_t scheduler_set_enabled(bool enabled);
bool scheduler_is_enabled(void);

// Internal helper functions
bool scheduler_is_schedule_due(feeding_schedule_t* schedule, struct tm* current_time);
uint8_t scheduler_get_next_schedule_id(void);
esp_err_t scheduler_save_to_nvs(void);
esp_err_t scheduler_load_from_nvs(void);

#ifdef __cplusplus
}
#endif

#endif // SCHEDULER_H