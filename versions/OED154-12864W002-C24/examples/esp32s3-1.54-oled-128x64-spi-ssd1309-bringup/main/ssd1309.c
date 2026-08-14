#include "ssd1309.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ssd1309";

static esp_lcd_panel_io_handle_t s_io;

static const uint8_t s_init_sequence[] = {
    0xAE,       /* Display OFF */
    0xA4,       /* ignores contents of display RAM */
    0xA6,       /* Normal display */
    0xA8, 0x3F, /* Multiplex ratio */
    0xD3, 0x00, /* Display offset */
    0x40,       /* Display start line */
    0xA1,       /* Segment remap */
    0xC8,       /* COM output scan direction */
    0xDA, 0x12, /* COM pins config */
    0x81, 0x7F, /* Contrast */
    0xA4,       /* Output follows RAM content */
    0xD5, 0x80, /* Clock divide */
    0x8D, 0x14, /* Charge pump */
    0xAF,       /* Display ON */
};

static esp_err_t ssd1309_write_cmd(uint8_t cmd)
{
    return esp_lcd_panel_io_tx_param(s_io, cmd, NULL, 0);
}

static void ssd1309_hw_reset(void)
{
    gpio_set_level(OLED_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(OLED_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static esp_err_t ssd1309_send_init_sequence(void)
{
    for (size_t i = 0; i < sizeof(s_init_sequence); i++) {
        ESP_RETURN_ON_ERROR(ssd1309_write_cmd(s_init_sequence[i]), TAG, "init cmd 0x%02x failed",
                            s_init_sequence[i]);
    }
    return ESP_OK;
}

esp_err_t ssd1309_flush(const uint8_t *fb)
{
    for (uint8_t page = 0; page < OLED_V_RES / 8; page++) {
        ESP_RETURN_ON_ERROR(ssd1309_write_cmd(0xB0 + page), TAG, "set page failed");
        ESP_RETURN_ON_ERROR(ssd1309_write_cmd(0x00), TAG, "set col low failed");
        ESP_RETURN_ON_ERROR(ssd1309_write_cmd(0x10), TAG, "set col high failed");
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_io_tx_color(s_io, -1, &fb[OLED_H_RES * page], OLED_H_RES), TAG,
            "write page data failed");
    }
    return ESP_OK;
}

esp_err_t ssd1309_init(void)
{
    gpio_config_t rst_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << OLED_PIN_RST,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&rst_cfg), TAG, "rst gpio config failed");
    gpio_set_level(OLED_PIN_RST, 1);

    spi_bus_config_t buscfg = {
        .sclk_io_num = OLED_PIN_SCLK,
        .mosi_io_num = OLED_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SSD1309_FB_SIZE,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(OLED_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG,
                        "spi_bus_initialize failed");

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = OLED_PIN_DC,
        .cs_gpio_num = OLED_PIN_CS,
        .pclk_hz = OLED_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 4,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)OLED_SPI_HOST, &io_config, &s_io), TAG,
        "esp_lcd_new_panel_io_spi failed");

    ssd1309_hw_reset();
    ESP_RETURN_ON_ERROR(ssd1309_send_init_sequence(), TAG, "init sequence failed");

    ESP_LOGI(TAG, "ready %dx%d", OLED_H_RES, OLED_V_RES);
    return ESP_OK;
}
