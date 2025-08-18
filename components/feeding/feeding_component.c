#include "feeding_component.h"
#include "iot_servo.h"
#include "esp_log.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

// Servo pins - PWM-capable on Seeed XIAO ESP32-C6
#define SERVO1_GPIO_PIN 16  // D6/TX0 - Better PWM support  
#define SERVO2_GPIO_PIN 17  // D7/RX0 - Better PWM support
#define FEEDING_TIMEOUT_MS 3000
#define BUTTON_DEBOUNCE_TIME_MS 50

// PWM configuration for MG90S servos
#define SERVO_PWM_FREQ 50        // 50Hz for servos
#define SERVO_PWM_RESOLUTION LEDC_TIMER_16_BIT
// MG90S optimized pulse widths for full 180° range
#define SERVO_MIN_PULSE_US 544   // 0.544ms = 0 degrees (MG90S)
#define SERVO_MAX_PULSE_US 2400  // 2.4ms = 180 degrees (MG90S)
#define SERVO_CENTER_PULSE_US 1472 // ~1.47ms = 90 degrees

// Two servo positions for feeding mechanism
#define POSITION_A_SERVO1 0.0f    // Ready/Dropping position
#define POSITION_A_SERVO2 180.0f
#define POSITION_B_SERVO1 180.0f  // Loading/Feeding position  
#define POSITION_B_SERVO2 0.0f

static const char* TAG = "FEEDING";
static feeding_handle_t feeding_handle = {0};
static bool servo_initialized = false;

static volatile uint32_t last_button_press_time = 0;
static QueueHandle_t button_event_queue;

// Direct PWM control functions
static uint32_t servo_angle_to_duty(float angle) {
    // Convert angle (0-180) to pulse width (1000-2000 us)
    uint32_t pulse_us = SERVO_MIN_PULSE_US + (angle / 180.0f) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
    // Convert pulse width to duty cycle for 16-bit resolution
    uint32_t period_us = 1000000 / SERVO_PWM_FREQ; // 20000us for 50Hz
    uint32_t duty = (pulse_us * ((1 << 16) - 1)) / period_us;
    return duty;
}

static esp_err_t servo_write_angle_direct(int servo_num, float angle) {
    ledc_channel_t channel = (servo_num == 0) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1;
    uint32_t duty = servo_angle_to_duty(angle);
    
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    if (ret != ESP_OK) return ret;
    
    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    
    // Calculate actual pulse width for debugging
    uint32_t period_us = 1000000 / SERVO_PWM_FREQ; // 20000us for 50Hz
    uint32_t pulse_us = (duty * period_us) / ((1 << 16) - 1);
    
    ESP_LOGI(TAG, "Servo %d: %.1f° → pulse %" PRIu32 "us (duty: %" PRIu32 ")", servo_num, angle, pulse_us, duty);
    return ret;
}

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
    
    // Initialize state only - servo init happens in feeding_start()
    feeding_handle.state = FEEDING_STATE_IDLE;
    feeding_handle.feeding_active = false;
    servo_initialized = false;
    
    ESP_LOGI(TAG, "Feeding component initialized - ready for operation");
    return ESP_OK;
}

void feeding_deinit(void)
{
    // Deinitialize servos completely on system shutdown
    if (servo_initialized) {
        // Stop PWM output
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        servo_initialized = false;
    }
    
    feeding_servo_power_disable();
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
    
    // Enable servo power FIRST
    feeding_servo_power_enable();
    
    // Wait for power stabilization
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize direct PWM control only once (first time)
    if (!servo_initialized) {
        // Configure LEDC timer
        ledc_timer_config_t timer_config = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = LEDC_TIMER_0,
            .duty_resolution = SERVO_PWM_RESOLUTION,
            .freq_hz = SERVO_PWM_FREQ,
            .clk_cfg = LEDC_AUTO_CLK
        };
        esp_err_t ret = ledc_timer_config(&timer_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
            feeding_servo_power_disable();
            return ret;
        }
        
        // Configure LEDC channels for servos
        ledc_channel_config_t servo1_config = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = SERVO1_GPIO_PIN,
            .duty = servo_angle_to_duty(POSITION_A_SERVO1), // Position A: Ready/Dropping
            .hpoint = 0
        };
        
        ledc_channel_config_t servo2_config = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_1,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = SERVO2_GPIO_PIN,
            .duty = servo_angle_to_duty(POSITION_A_SERVO2), // Position A: Ready/Dropping
            .hpoint = 0
        };
        
        ret = ledc_channel_config(&servo1_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Servo 1 channel config failed: %s", esp_err_to_name(ret));
            feeding_servo_power_disable();
            return ret;
        }
        
        ret = ledc_channel_config(&servo2_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Servo 2 channel config failed: %s", esp_err_to_name(ret));
            feeding_servo_power_disable();
            return ret;
        }
        
        servo_initialized = true;
        ESP_LOGI(TAG, "Direct PWM servos initialized for first time");
    } else {
        ESP_LOGI(TAG, "Servos already initialized, power enabled");
    }
    
    // CRITICAL: Send initial PWM signal immediately to prevent MG90S ticking
    // Set servos to Position A (Ready/Dropping)
    servo_write_angle_direct(0, POSITION_A_SERVO1);
    servo_write_angle_direct(1, POSITION_A_SERVO2);
    ESP_LOGI(TAG, "Servos set to Position A (Ready/Dropping)");
    
    feeding_handle.state = FEEDING_STATE_POSITION_B;
    feeding_handle.feeding_active = true;
    feeding_handle.state_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    ESP_LOGI(TAG, "Feeding cycle started");
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
        case FEEDING_STATE_POSITION_B:
            ESP_LOGI(TAG, "Moving to Position B (Loading/Feeding)");
            servo_write_angle_direct(0, POSITION_B_SERVO1);
            servo_write_angle_direct(1, POSITION_B_SERVO2);
            feeding_handle.state = FEEDING_STATE_POSITION_A;
            feeding_handle.state_start_time = current_time;
            break;
            
        case FEEDING_STATE_POSITION_A:
            if (elapsed_time >= FEEDING_TIMEOUT_MS) {
                ESP_LOGI(TAG, "Moving to Position A (Ready/Dropping)");
                servo_write_angle_direct(0, POSITION_A_SERVO1);
                servo_write_angle_direct(1, POSITION_A_SERVO2);
                feeding_handle.state = FEEDING_STATE_COMPLETE;
                feeding_handle.state_start_time = current_time;
            }
            break;
            
        case FEEDING_STATE_COMPLETE:
            if (elapsed_time >= FEEDING_TIMEOUT_MS) {
                ESP_LOGI(TAG, "Feeding cycle complete - ready for next feeding");
                
                // Keep servos initialized, only disable power for energy saving
                feeding_servo_power_disable();
                
                feeding_handle.state = FEEDING_STATE_IDLE;
                feeding_handle.feeding_active = false;
            }
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
    ESP_LOGI(TAG, "Servo power control initialized successfully (S8550 PNP)");
    return ESP_OK;
}

void feeding_servo_power_enable(void)
{
    gpio_set_level(SERVO_POWER_CONTROL_GPIO, 0);  // LOW for PNP transistor (S8550)
    int level = gpio_get_level(SERVO_POWER_CONTROL_GPIO);
    ESP_LOGI(TAG, "Servo power enable: GPIO%d set to %d (PNP: LOW=ON)", SERVO_POWER_CONTROL_GPIO, level);
}

void feeding_servo_power_disable(void)
{
    gpio_set_level(SERVO_POWER_CONTROL_GPIO, 1);  // HIGH for PNP transistor (S8550)
    int level = gpio_get_level(SERVO_POWER_CONTROL_GPIO);
    ESP_LOGI(TAG, "Servo power disable: GPIO%d set to %d (PNP: HIGH=OFF)", SERVO_POWER_CONTROL_GPIO, level);
}