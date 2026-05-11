#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//
#include "lv_demos.h"
//
#include "bsp/esp-bsp.h"
#define LOG_MEM_INFO (0)

void app_main(void)
{
    ESP_LOGI("MAIN", "Main Started");
    /* Initialize display and LVGL */
    bsp_display_start();
    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    bsp_display_lock(0);
    lv_demo_widgets(); /* A widgets example */
    bsp_display_unlock();

    while (1)
    {
        ESP_LOGI("MAIN", "Running...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
