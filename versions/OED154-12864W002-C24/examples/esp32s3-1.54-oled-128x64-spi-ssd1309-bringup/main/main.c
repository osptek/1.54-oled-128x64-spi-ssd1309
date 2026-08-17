/*
 * SPDX-FileCopyrightText: Copyright 2026 OSPTEK
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * https://github.com/osptek
 */

#include "esp_err.h"
#include "ssd1309.h"
#include "ssd1309_anim.h"

void app_main(void)
{
    ESP_ERROR_CHECK(ssd1309_init());
    ESP_ERROR_CHECK(ssd1309_anim_start());
}
