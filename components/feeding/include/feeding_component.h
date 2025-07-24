#ifndef FEEDING_COMPONENT_H
#define FEEDING_COMPONENT_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    FEEDING_STATE_IDLE,
    FEEDING_STATE_INIT,
    FEEDING_STATE_LOADING,
    FEEDING_STATE_EMPTYING,
    FEEDING_STATE_READY
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

#endif