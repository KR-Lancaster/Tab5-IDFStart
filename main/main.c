#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"

void app_main(void)
{
    ESP_LOGI("MAIN", "Main Started");
    /* Initialize display and LVGL */
    bsp_display_start();
    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    while (1)
    {
        ESP_LOGI("MAIN", "Running...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
