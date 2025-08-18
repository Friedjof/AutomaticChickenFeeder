#ifndef FEEDING_COMPONENT_H
#define FEEDING_COMPONENT_H

#include "esp_err.h"
#include <stdbool.h>

// Pin definitions for Seeed XIAO ESP32-C6
#define MANUAL_FEED_BUTTON_GPIO 1   // D1 - Interrupt-capable, ULP-accessible
#define SERVO_POWER_CONTROL_GPIO 20 // Test: GPIO20 instead of GPIO21

typedef enum {
    FEEDING_STATE_IDLE,        // Ready, waiting for trigger
    FEEDING_STATE_POSITION_B,  // Moving to/holding Position B (Loading)
    FEEDING_STATE_POSITION_A,  // Moving to/holding Position A (Ready/Dropping)
    FEEDING_STATE_COMPLETE     // Feeding cycle finished
} feeding_state_t;

typedef struct {
    feeding_state_t state;
    uint32_t state_start_time;
    bool feeding_active;
} feeding_handle_t;

esp_err_t feeding_init(void);
void feeding_deinit(void);
esp_err_t feeding_start(void);
void feeding_process(void);
feeding_state_t feeding_get_state(void);
bool feeding_is_ready(void);

esp_err_t feeding_button_init(void);
void feeding_button_deinit(void);
void feeding_handle_button_events(void);

esp_err_t feeding_servo_power_init(void);
void feeding_servo_power_enable(void);
void feeding_servo_power_disable(void);

#endif