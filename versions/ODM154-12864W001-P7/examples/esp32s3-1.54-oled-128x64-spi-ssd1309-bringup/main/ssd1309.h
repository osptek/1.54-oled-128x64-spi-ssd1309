#pragma once

#include <stdint.h>

#include "board_config.h"
#include "esp_err.h"

#define SSD1309_FB_SIZE (OLED_H_RES * OLED_V_RES / 8)

esp_err_t ssd1309_init(void);
esp_err_t ssd1309_flush(const uint8_t *fb);
