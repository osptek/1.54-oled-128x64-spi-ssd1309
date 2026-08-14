#pragma once

/* 1.54" 128x64 SSD1309 SPI (丝印 SCL/SDA 对应 SPI 时钟与 MOSI) */
#define OLED_SPI_HOST       SPI2_HOST

#define OLED_PIN_SCLK       9   /* SCL */
#define OLED_PIN_MOSI       10  /* SDA */
#define OLED_PIN_RST        11
#define OLED_PIN_DC         12
#define OLED_PIN_CS         13

#define OLED_H_RES          128
#define OLED_V_RES          64
#define OLED_PIXEL_CLOCK_HZ (10 * 1000 * 1000)

