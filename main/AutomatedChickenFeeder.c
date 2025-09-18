#include "esp_check.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"

#define HA_ESP_LIGHT_ENDPOINT 1

static const char *TAG = "ZB_SLEEP";

// Callback: enter light sleep when Zigbee stack reports it can sleep
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    uint32_t *sig = signal_struct->p_app_signal;
    esp_err_t status = signal_struct->esp_err_status;
    switch (*sig) {
        case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
            ESP_LOGI(TAG, "Zigbee can sleep");
            // Put the radio/CPU into light sleep; wakes on message or timer
            esp_zb_sleep_now();
            break;
        // handle other signals (initialization, network steering, etc.)
        default:
            break;
    }
}

// Optional: configure power management (single CPU freq., enable light sleep)
static esp_err_t power_save_init(void) {
    esp_err_t rc = ESP_OK;
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .light_sleep_enable = true,
    };
    rc = esp_pm_configure(&pm_config);
    return rc;
}

void app_main(void) {
    // Standard NVS init
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize power management to allow light sleep
    power_save_init();

    // Zigbee platform configuration
    esp_zb_platform_config_t cfg = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config  = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&cfg));

    // Enable Zigbee light sleep before initializing the stack
    esp_zb_sleep_enable(true);

    // Set Zigbee end‑device configuration (keep_alive default can be adjusted)
    esp_zb_cfg_t zb_cfg = ESP_ZB_ZED_CONFIG();
    zb_cfg.keep_alive = 7500; // keep alive interval in ms
    esp_zb_init(&zb_cfg);

    // Register your endpoints (e.g. On/Off light) and callbacks …
    esp_zb_on_off_light_cfg_t light_cfg = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
    esp_zb_ep_list_t *ep = esp_zb_on_off_light_ep_create(HA_ESP_LIGHT_ENDPOINT, &light_cfg);
    esp_zb_device_register(ep);
    esp_zb_core_action_handler_register(/* your handler */);

    // Start Zigbee
    ESP_ERROR_CHECK(esp_zb_start(false));
    // Main loop handles signals and sleeps when possible
    esp_zb_stack_main_loop();
}
