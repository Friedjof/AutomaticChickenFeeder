#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "feeding_component.h"
#include "clock_service.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

static const char* TAG = "CHICKEN_FEEDER";
static uint32_t last_time_print = 0;

// I2C Scanner für DS3231 Debug
void i2c_scanner(void) {
    const char* SCANNER_TAG = "I2C_SCANNER";
    ESP_LOGI(SCANNER_TAG, "Starting I2C scanner...");
    
    // I2C Master konfigurieren
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 22,  // GPIO22 (D4)  
        .scl_io_num = 23,  // GPIO23 (D5)
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(SCANNER_TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(SCANNER_TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(SCANNER_TAG, "I2C driver installed successfully");
    ESP_LOGI(SCANNER_TAG, "Scanning I2C bus...");
    
    int devices_found = 0;
    
    for (int addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            ESP_LOGI(SCANNER_TAG, "Found device at address 0x%02X", addr);
            if (addr == 0x68) {
                ESP_LOGI(SCANNER_TAG, "  -> DS3231 RTC detected!");
            }
            devices_found++;
        }
    }
    
    if (devices_found == 0) {
        ESP_LOGE(SCANNER_TAG, "No I2C devices found!");
        ESP_LOGE(SCANNER_TAG, "Check connections:");
        ESP_LOGE(SCANNER_TAG, "  VCC -> 3V3");
        ESP_LOGE(SCANNER_TAG, "  GND -> GND");
        ESP_LOGE(SCANNER_TAG, "  SDA -> D4 (GPIO5)");
        ESP_LOGE(SCANNER_TAG, "  SCL -> D5 (GPIO6)");
        ESP_LOGE(SCANNER_TAG, "  Pull-up resistors (4.7kΩ) on SDA/SCL?");
        ESP_LOGE(SCANNER_TAG, "  CR2032 battery inserted?");
    } else {
        ESP_LOGI(SCANNER_TAG, "Found %d I2C device(s)", devices_found);
    }
    
    i2c_driver_delete(I2C_NUM_0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Automatic Chicken Feeder gestartet");
    
    // TEMPORÄR: I2C Scanner zum Testen der DS3231 Verbindung
    ESP_LOGI(TAG, "Running I2C scanner to check DS3231 connection...");
    i2c_scanner();
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Kurz warten
    
    // Initialize clock service first
    esp_err_t ret = clock_service_init();
    bool clock_service_available = (ret == ESP_OK);
    if (!clock_service_available) {
        ESP_LOGE(TAG, "Clock service init failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Continuing without DS3231 RTC - using demo feeding mode");
    }
    
    ret = feeding_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Feeding component init failed: %s", esp_err_to_name(ret));
        if (clock_service_available) {
            clock_service_deinit();
        }
        return;
    }
    
    ret = feeding_button_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed: %s", esp_err_to_name(ret));
        feeding_deinit();
        if (clock_service_available) {
            clock_service_deinit();
        }
        return;
    }
    
    ret = feeding_servo_power_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Servo power control init failed: %s", esp_err_to_name(ret));
        feeding_button_deinit();
        feeding_deinit();
        if (clock_service_available) {
            clock_service_deinit();
        }
        return;
    }
    
    // Initialize wake interrupt for deep sleep (only if RTC available)
    if (clock_service_available) {
        ret = clock_service_init_wake_interrupt();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init wake interrupt: %s", esp_err_to_name(ret));
            clock_service_available = false;  // Disable RTC features
        } else {
            ESP_LOGI(TAG, "Wake interrupt initialized for deep sleep");
            
            // Configure sleep mode for network connectivity
            // Options: CLOCK_SLEEP_MODE_DEEP, CLOCK_SLEEP_MODE_LIGHT, CLOCK_SLEEP_MODE_MODEM
            ret = clock_service_set_sleep_mode(CLOCK_SLEEP_MODE_LIGHT); // WiFi preserved
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Sleep mode configured: Light Sleep (WiFi preserved)");
            }
        }
    }
    
    ESP_LOGI(TAG, "All components initialized successfully");
    ESP_LOGI(TAG, "System ready - manual feeding available via button");
    
    // Check if we woke up from DS3231 alarm (only if RTC available)
    if (clock_service_available && clock_service_is_wake_from_alarm()) {
        ESP_LOGI(TAG, "=== WOKE UP FROM DS3231 ALARM ===");
        ESP_LOGI(TAG, "Starting automatic feeding cycle...");
        
        // Clear the alarm
        clock_service_clear_wake_alarm();
        
        // Trigger feeding
        if (feeding_is_ready()) {
            ret = feeding_start();
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Feeding started successfully");
            } else {
                ESP_LOGE(TAG, "Failed to start feeding: %s", esp_err_to_name(ret));
            }
        }
        
        // Wait for feeding to complete and then set next alarm
        while (!feeding_is_ready()) {
            feeding_process();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        ESP_LOGI(TAG, "Feeding completed! Setting next wake alarm for 10 seconds...");
        
        // Set next alarm for 10 seconds from now
        ret = clock_service_setup_wake_alarm(10);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Next alarm set, entering deep sleep...");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Allow log to be printed
            clock_service_enter_deep_sleep();  // This will not return
        } else {
            ESP_LOGE(TAG, "Failed to set next alarm: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(TAG, "Normal startup (not from alarm)");
        if (clock_service_available) {
            ESP_LOGI(TAG, "RTC available - will demonstrate sleep cycle after 10 seconds...");
        } else {
            ESP_LOGI(TAG, "No RTC - will demonstrate timer-based feeding after 15 seconds...");
        }
    }
    
    // Check existing schedules and clear if too many (demo cleanup)
    if (clock_service_available) {
        feeding_schedule_t schedules[16];
        uint8_t schedule_count = 0;
        ret = clock_service_get_schedules(schedules, &schedule_count);
        
        ESP_LOGI(TAG, "Found %d existing schedules in NVS", schedule_count);
        
        // Clear all schedules if we have too many (demo cleanup)
        if (schedule_count >= 16) {
            ESP_LOGW(TAG, "Too many schedules (%d/16), clearing all for demo", schedule_count);
            ret = clock_service_clear_schedules();
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "All schedules cleared");
                schedule_count = 0;
            }
        }
        
        // Add test schedule only if we don't have many
        if (schedule_count < 3) {
            feeding_schedule_t test_schedule = {
                .id = 0,  // Auto-assign ID
                .hour = 8,
                .minute = 0,
                .weekdays = CLOCK_DAILY,
                .enabled = true,
                .name = "Morning Feed"
            };
            
            ret = clock_service_add_schedule(&test_schedule);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Added test schedule: Daily at 08:00");
            } else {
                ESP_LOGE(TAG, "Failed to add test schedule: %s", esp_err_to_name(ret));
            }
        } else {
            ESP_LOGI(TAG, "Skipping test schedule creation - already have %d schedules", schedule_count);
        }
    }
    
    uint32_t startup_time = xTaskGetTickCount();
    bool sleep_demo_started = false;
    bool timer_feeding_started = false;
    uint32_t feeding_counter = 0;
    
    while(1) {
        uint32_t current_tick = xTaskGetTickCount();
        
        // Process feeding and clock services
        feeding_process();
        feeding_handle_button_events();
        if (clock_service_available) {
            clock_service_process();
        }
        
        // RTC MODE: Demonstrate sleep cycle after 10 seconds
        if (clock_service_available && !sleep_demo_started && (current_tick - startup_time) >= pdMS_TO_TICKS(10000)) {
            sleep_demo_started = true;
            ESP_LOGI(TAG, "=== STARTING DEEP SLEEP DEMO ===");
            ESP_LOGI(TAG, "Setting wake alarm for 10 seconds from now...");
            
            ret = clock_service_setup_wake_alarm(10);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Alarm set! Entering deep sleep in 2 seconds...");
                vTaskDelay(pdMS_TO_TICKS(2000));
                clock_service_enter_deep_sleep();  // This will not return
            } else {
                ESP_LOGE(TAG, "Failed to set demo alarm: %s", esp_err_to_name(ret));
                sleep_demo_started = false; // Reset to try again
            }
        }
        
        // NO RTC MODE: Timer-based feeding every 15 seconds
        if (!clock_service_available && !timer_feeding_started && (current_tick - startup_time) >= pdMS_TO_TICKS(15000)) {
            timer_feeding_started = true;
            ESP_LOGI(TAG, "=== STARTING TIMER-BASED AUTOMATIC FEEDING ===");
            ESP_LOGI(TAG, "Will feed every 15 seconds (demo mode)");
        }
        
        if (!clock_service_available && timer_feeding_started) {
            // Automatic feeding every 15 seconds without RTC
            if ((current_tick - startup_time) >= pdMS_TO_TICKS(15000 + (feeding_counter * 15000))) {
                if (feeding_is_ready()) {
                    ESP_LOGI(TAG, "=== AUTOMATIC TIMER FEEDING #%lu ===", feeding_counter + 1);
                    ret = feeding_start();
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "Timer-based feeding started successfully");
                        feeding_counter++;
                    } else {
                        ESP_LOGE(TAG, "Failed to start timer feeding: %s", esp_err_to_name(ret));
                    }
                } else {
                    ESP_LOGW(TAG, "Feeding system not ready, skipping automatic feed");
                }
            }
        }
        
        // Print status every 5 seconds 
        if (current_tick - last_time_print >= pdMS_TO_TICKS(5000)) {
            if (clock_service_available) {
                struct tm current_time;
                if (clock_service_get_time(&current_time) == ESP_OK) {
                    char time_str[32];
                    if (clock_service_format_time_string(&current_time, time_str, sizeof(time_str)) == ESP_OK) {
                        ESP_LOGI(TAG, "Current time: %s (%s)", time_str, 
                                 clock_service_is_rtc_running() ? "RTC OK" : "RTC ERROR");
                        
                        // Also show temperature
                        float temperature;
                        if (clock_service_get_temperature(&temperature) == ESP_OK) {
                            ESP_LOGI(TAG, "DS3231 Temperature: %.1f°C", temperature);
                        }
                    }
                    
                    // Show next feeding time
                    struct tm next_feeding;
                    if (clock_service_get_next_feeding_time(&next_feeding) == ESP_OK) {
                        char next_time_str[32];
                        if (clock_service_format_time_string(&next_feeding, next_time_str, sizeof(next_time_str)) == ESP_OK) {
                            ESP_LOGI(TAG, "Next feeding: %s", next_time_str);
                        }
                    }
                    
                    if (!sleep_demo_started) {
                        uint32_t seconds_until_demo = (10000 - (current_tick - startup_time)) / 1000;
                        ESP_LOGI(TAG, "Deep sleep demo starts in %lu seconds...", seconds_until_demo);
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to read RTC time");
                }
            } else {
                // No RTC mode - show timer-based status
                ESP_LOGI(TAG, "Timer mode active (no RTC) - Uptime: %lu seconds", 
                         (current_tick - startup_time) / 1000);
                ESP_LOGI(TAG, "Feeding count: %lu", feeding_counter);
                
                if (!timer_feeding_started) {
                    uint32_t seconds_until_feeding = (15000 - (current_tick - startup_time)) / 1000;
                    ESP_LOGI(TAG, "Automatic feeding starts in %lu seconds...", seconds_until_feeding);
                } else {
                    uint32_t next_feeding_time = 15000 + (feeding_counter * 15000);
                    if ((current_tick - startup_time) < next_feeding_time) {
                        uint32_t seconds_until_next = (next_feeding_time - (current_tick - startup_time)) / 1000;
                        ESP_LOGI(TAG, "Next feeding in %lu seconds...", seconds_until_next);
                    }
                }
            }
            last_time_print = current_tick;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
