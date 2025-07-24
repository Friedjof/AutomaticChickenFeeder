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
    
    ESP_LOGI(TAG, "Feeding component initialized successfully");
    
    uint32_t feed_counter = 0;
    
    while(1) {
        feeding_process();
        
        if (feeding_is_ready()) {
            feed_counter++;
            ESP_LOGI(TAG, "Starting feeding cycle #%lu", feed_counter);
            feeding_start();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
