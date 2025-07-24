#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "feeding_component.h"

static const char* TAG = "CHICKEN_FEEDER";

void app_main(void)
{
    ESP_LOGI(TAG, "Automatic Chicken Feeder gestartet");
    
    esp_err_t ret = feeding_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Feeding component init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = feeding_button_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %s", esp_err_to_name(ret));
        feeding_deinit();
        return;
    }
    
    ret = feeding_servo_power_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo power control init failed: %s", esp_err_to_name(ret));
        feeding_button_deinit();
        feeding_deinit();
        return;
    }
    
    ESP_LOGI(TAG, "Feeding component, button, and servo power control initialized successfully");
    ESP_LOGI(TAG, "System ready - manual feeding available via button");
    
    while(1) {
        feeding_process();
        feeding_handle_button_events();
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
