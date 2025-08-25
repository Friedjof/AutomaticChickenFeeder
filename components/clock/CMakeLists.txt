idf_component_register(
    SRCS "src/clock_service.c" 
         "src/rtc_driver.c" 
         "src/scheduler.c" 
         "src/time_manager.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private_include"
    REQUIRES "driver" "esp_timer" "nvs_flash"
)