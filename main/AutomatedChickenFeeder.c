#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "iot_servo.h"

#define SERVO1_GPIO_PIN 1
#define SERVO2_GPIO_PIN 2

static const char* TAG = "SERVO_CONTROL";

void app_main(void)
{
    ESP_LOGI(TAG, "Servo Motor Test gestartet - GPIO1 & GPIO2");
    
    // Servo Konfiguration
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
    
    // Servo initialisieren
    esp_err_t ret = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo Init fehlgeschlagen: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Servos erfolgreich initialisiert");
    
    while(1) {
        ESP_LOGI(TAG, "Beide Servos: 0°");
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 0.0f);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 180.0f);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        ESP_LOGI(TAG, "Beide Servos: 180°");
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 0, 180.0f);
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, 1, 0.0f);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
