idf_component_register(SRCS "main.c" "ssd1309.c" "ssd1309_anim.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd driver)
