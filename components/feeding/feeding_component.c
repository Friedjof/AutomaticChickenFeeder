#include "feeding_component.h"
#include "iot_servo.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#define SERVO1_GPIO_PIN 1
#define SERVO2_GPIO_PIN 2
#define FEEDING_TIMEOUT_MS 3000

static const char* TAG = "FEEDING";
static feeding_handle_t feeding_handle = {0};

esp_err_t feeding_init(void)
{
    ESP_LOGI(TAG, "Initializing feeding component");
    
    servo_config_t servo_cfg = {
        .max_angle = 180,
        .min_width_us = 500,
        .max_width_us = 2500,
        .freq = 50,
        .timer_number = LEDC_TIMER_0,
        .channels = {
            .servo_pin = {SERVO1_GPIO_PIN, SERVO2_GPIO_PIN},
            .ch = {LEDC_CHANNEL_0, LEDC_CHANNEL_1},
        },
        .channel_number = 2,
    };
    
    esp_err_t ret = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 0.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 180.0f);
    feeding_handle.state = FEEDING_STATE_IDLE;
    feeding_handle.feeding_active = false;
    
    ESP_LOGI(TAG, "Feeding component initialized - servos in loading position");
    return ESP_OK;
}

void feeding_deinit(void)
{
    iot_servo_deinit(LEDC_LOW_SPEED_MODE);
    feeding_handle.state = FEEDING_STATE_IDLE;
    feeding_handle.feeding_active = false;
}

esp_err_t feeding_start(void)
{
    if (feeding_handle.feeding_active) {
        ESP_LOGW(TAG, "Feeding already in progress");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Starting feeding cycle");
    feeding_handle.state = FEEDING_STATE_INIT;
    feeding_handle.feeding_active = true;
    feeding_handle.state_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    return ESP_OK;
}

void feeding_process(void)
{
    if (!feeding_handle.feeding_active) {
        return;
    }
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t elapsed_time = current_time - feeding_handle.state_start_time;
    
    switch (feeding_handle.state) {
        case FEEDING_STATE_INIT:
            ESP_LOGI(TAG, "Moving to emptying position");
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 180.0f);
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 0.0f);
            feeding_handle.state = FEEDING_STATE_EMPTYING;
            feeding_handle.state_start_time = current_time;
            break;
            
        case FEEDING_STATE_EMPTYING:
            if (elapsed_time >= FEEDING_TIMEOUT_MS) {
                ESP_LOGI(TAG, "Moving to loading position");
                iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 0.0f);
                iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 180.0f);
                feeding_handle.state = FEEDING_STATE_LOADING;
                feeding_handle.state_start_time = current_time;
            }
            break;
            
        case FEEDING_STATE_LOADING:
            if (elapsed_time >= FEEDING_TIMEOUT_MS) {
                ESP_LOGI(TAG, "Feeding complete - ready for next cycle");
                feeding_handle.state = FEEDING_STATE_READY;
                feeding_handle.feeding_active = false;
            }
            break;
            
        case FEEDING_STATE_READY:
            feeding_handle.state = FEEDING_STATE_IDLE;
            break;
            
        default:
            break;
    }
}

feeding_state_t feeding_get_state(void)
{
    return feeding_handle.state;
}

bool feeding_is_ready(void)
{
    return !feeding_handle.feeding_active && feeding_handle.state == FEEDING_STATE_IDLE;
}