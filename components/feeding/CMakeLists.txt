idf_component_register(SRCS "feeding_component.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_common freertos log)