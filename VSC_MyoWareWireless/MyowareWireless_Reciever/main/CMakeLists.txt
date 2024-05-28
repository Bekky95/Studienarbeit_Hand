idf_component_register(SRCS "main.c"
                    REQUIRES driver
                    REQUIRES bt
                    REQUIRES nvs_flash
                    REQUIRES servo
                    REQUIRES esp_timer
                    INCLUDE_DIRS ".")
