idf_component_register(SRCS "main.c"
                    REQUIRES driver
                    REQUIRES bt
                    REQUIRES nvs_flash
                    INCLUDE_DIRS ".")
