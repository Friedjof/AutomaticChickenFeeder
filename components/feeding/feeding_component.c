#include "feeding_component.h"
#include "iot_servo.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

#define SERVO1_GPIO_PIN 1
#define SERVO2_GPIO_PIN 2
#define FEEDING_TIMEOUT_MS 3000
#define BUTTON_DEBOUNCE_TIME_MS 50

static const char* TAG = "FEEDING";
static feeding_handle_t feeding_handle = {0};

static volatile uint32_t last_button_press_time = 0;
static QueueHandle_t button_event_queue;

static void IRAM_ATTR manual_feed_button_isr_handler(void* arg)
{
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (current_time - last_button_press_time > BUTTON_DEBOUNCE_TIME_MS) {
        last_button_press_time = current_time;
        uint32_t button_event = 1;
        BaseType_t high_task_awoken = pdFALSE;
        xQueueSendFromISR(button_event_queue, &button_event, &high_task_awoken);
        if (high_task_awoken) {
            portYIELD_FROM_ISR();
        }
    }
}

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
    
    feeding_handle.state = FEEDING_STATE_IDLE;
    feeding_handle.feeding_active = false;
    
    ESP_LOGI(TAG, "Feeding component initialized - servos ready for manual operation");
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
    feeding_servo_power_enable();
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
                feeding_servo_power_disable();
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

esp_err_t feeding_button_init(void)
{
    ESP_LOGI(TAG, "Initializing manual feed button on GPIO %d", MANUAL_FEED_BUTTON_GPIO);
    
    button_event_queue = xQueueCreate(10, sizeof(uint32_t));
    if (button_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button event queue");
        return ESP_ERR_NO_MEM;
    }
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << MANUAL_FEED_BUTTON_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        vQueueDelete(button_event_queue);
        return ret;
    }
    
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(ret));
        vQueueDelete(button_event_queue);
        return ret;
    }
    
    ret = gpio_isr_handler_add(MANUAL_FEED_BUTTON_GPIO, manual_feed_button_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO ISR handler add failed: %s", esp_err_to_name(ret));
        vQueueDelete(button_event_queue);
        return ret;
    }
    
    ESP_LOGI(TAG, "Manual feed button initialized successfully");
    return ESP_OK;
}

void feeding_button_deinit(void)
{
    gpio_isr_handler_remove(MANUAL_FEED_BUTTON_GPIO);
    if (button_event_queue != NULL) {
        vQueueDelete(button_event_queue);
        button_event_queue = NULL;
    }
    ESP_LOGI(TAG, "Manual feed button deinitialized");
}

void feeding_handle_button_events(void)
{
    uint32_t button_event;
    if (button_event_queue != NULL && xQueueReceive(button_event_queue, &button_event, 0) == pdTRUE) {
        ESP_LOGI(TAG, "Manual feed button pressed - triggering feeding sequence");
        
        if (feeding_handle.feeding_active) {
            ESP_LOGW(TAG, "Feeding already in progress, ignoring button press");
        } else {
            esp_err_t ret = feeding_start();
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Manual feeding started successfully");
            } else {
                ESP_LOGE(TAG, "Failed to start manual feeding: %s", esp_err_to_name(ret));
            }
        }
    }
}

esp_err_t feeding_servo_power_init(void)
{
    ESP_LOGI(TAG, "Initializing servo power control on GPIO %d", SERVO_POWER_CONTROL_GPIO);
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << SERVO_POWER_CONTROL_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo power control GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    feeding_servo_power_disable();
    ESP_LOGI(TAG, "Servo power control initialized successfully");
    return ESP_OK;
}

void feeding_servo_power_enable(void)
{
    gpio_set_level(SERVO_POWER_CONTROL_GPIO, 1);
    ESP_LOGD(TAG, "Servo power enabled");
}

void feeding_servo_power_disable(void)
{
    gpio_set_level(SERVO_POWER_CONTROL_GPIO, 0);
    ESP_LOGD(TAG, "Servo power disabled");
}