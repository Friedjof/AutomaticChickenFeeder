idf_component_register(SRCS "AutomatedChickenFeeder.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_pm espressif__esp-zigbee-lib espressif__esp-zboss-lib)
