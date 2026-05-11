#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "app_images.h"

#define TAG "MAIN"
#define SIDEBAR_W 250

// Ring buffer for log lines (fixed size, no dynamic allocation)
#define LOG_LINE_MAX_LEN 128
#define LOG_LINE_COUNT 10 // keep last 15 lines (reduced further)

static char log_lines[LOG_LINE_COUNT][LOG_LINE_MAX_LEN];
static int log_head = 0;
static int log_count = 0;

/* ========================================================= */
/* LOG SYSTEM – writes to ring buffer                        */
/* ========================================================= */

static void add_log_line(const char *msg)
{
    strncpy(log_lines[log_head], msg, LOG_LINE_MAX_LEN - 1);
    log_lines[log_head][LOG_LINE_MAX_LEN - 1] = '\0';
    log_head = (log_head + 1) % LOG_LINE_COUNT;
    if (log_count < LOG_LINE_COUNT)
        log_count++;
}

static int custom_log_vprintf(const char *fmt, va_list args)
{
    static char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    add_log_line(buf);
    return strlen(buf);
}

/* ========================================================= */
/* UI OBJECTS                                                */
/* ========================================================= */

static lv_obj_t *content_area;
static lv_obj_t *console_panel;
static lv_obj_t *music_panel;
static lv_obj_t *nes_panel;
static lv_obj_t *console_textarea;

/* ========================================================= */
/* APP STATE                                                 */
/* ========================================================= */

typedef enum
{
    APP_CONSOLE = 0,
    APP_MUSIC,
    APP_NES,
} app_id_t;

static volatile app_id_t current_app = APP_CONSOLE;

/* ========================================================= */
/* REFRESH CONSOLE TEXT (rebuild from ring buffer)           */
/* ========================================================= */

static void refresh_console_text(void)
{
    if (!console_textarea)
        return;

    // Static buffer – not on stack
    static char buffer[LOG_LINE_COUNT * (LOG_LINE_MAX_LEN + 1) + 1];
    char *ptr = buffer;
    int start = (log_head - log_count + LOG_LINE_COUNT) % LOG_LINE_COUNT;
    for (int i = 0; i < log_count; i++)
    {
        int idx = (start + i) % LOG_LINE_COUNT;
        ptr += snprintf(ptr, sizeof(buffer) - (ptr - buffer), "%s\n", log_lines[idx]);
    }
    lv_textarea_set_text(console_textarea, buffer);
    lv_textarea_set_cursor_pos(console_textarea, LV_TEXTAREA_CURSOR_LAST);
}

/* ========================================================= */
/* BACKGROUND TASKS                                          */
/* ========================================================= */

static void console_task(void *arg)
{
    while (1)
    {
        // ESP_LOGI("CONSOLE", "Console running");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void music_task(void *arg)
{
    int c = 0;
    while (1)
    {
        ESP_LOGI("MUSIC", "Tick %d", c++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void nes_task(void *arg)
{
    int f = 0;
    while (1)
    {
        ESP_LOGI("NES", "Frame %d", f++);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ========================================================= */
/* UI SWITCH                                                 */
/* ========================================================= */

static void show_only(lv_obj_t *panel)
{
    lv_obj_add_flag(console_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(music_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(nes_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
}

static void switch_app(app_id_t app)
{
    current_app = app;
    switch (app)
    {
    case APP_CONSOLE:
        show_only(console_panel);
        if (lvgl_port_lock(0))
        {
            refresh_console_text(); // refresh when switching to console
            lvgl_port_unlock();
        }
        break;
    case APP_MUSIC:
        show_only(music_panel);
        break;
    case APP_NES:
        show_only(nes_panel);
        break;
    }
}

/* ========================================================= */
/* BUTTON EVENTS                                             */
/* ========================================================= */

static void btn_console_event(lv_event_t *e) { switch_app(APP_CONSOLE); }
static void btn_music_event(lv_event_t *e) { switch_app(APP_MUSIC); }
static void btn_nes_event(lv_event_t *e) { switch_app(APP_NES); }

/* ========================================================= */
/* UI CREATION                                               */
/* ========================================================= */

static lv_obj_t *create_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_width(btn, SIDEBAR_W - 50);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

static void create_ui(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    lv_display_set_rotation(lv_display_get_default(), LV_DISPLAY_ROTATION_90);

    lv_coord_t w = lv_display_get_horizontal_resolution(lv_display_get_default());
    lv_coord_t h = lv_display_get_vertical_resolution(lv_display_get_default());
    ESP_LOGI(TAG, "Display: %d x %d", (int)w, (int)h);

    /* Sidebar */
    lv_obj_t *sidebar = lv_obj_create(screen);
    lv_obj_set_size(sidebar, SIDEBAR_W, h);
    lv_obj_align(sidebar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);

    create_button(sidebar, "Console", btn_console_event);
    create_button(sidebar, "Music", btn_music_event);
    create_button(sidebar, "NES", btn_nes_event);

    /* Content area */
    content_area = lv_obj_create(screen);
    lv_obj_set_size(content_area, w - SIDEBAR_W, h);
    lv_obj_align(content_area, LV_ALIGN_RIGHT_MID, 0, 0);

    /* Console panel */
    console_panel = lv_obj_create(content_area);
    lv_obj_set_size(console_panel, LV_PCT(100), LV_PCT(100));

    console_textarea = lv_textarea_create(console_panel);
    lv_obj_set_size(console_textarea, LV_PCT(100), LV_PCT(100));
    lv_textarea_set_one_line(console_textarea, false);
    // Do NOT call refresh_console_text() here – the refresh task will handle it

    /* Music panel */
    music_panel = lv_obj_create(content_area);
    lv_obj_set_size(music_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_t *img1 = lv_img_create(music_panel);
    lv_img_set_src(img1, &decepticon);
    lv_obj_center(img1);

    /* NES panel */
    nes_panel = lv_obj_create(content_area);
    lv_obj_set_size(nes_panel, LV_PCT(100), LV_PCT(100));
    lv_obj_t *img2 = lv_img_create(nes_panel);
    lv_img_set_src(img2, &autobot);
    lv_obj_center(img2);

    switch_app(APP_CONSOLE);
}

/* ========================================================= */
/* TASK THAT PERIODICALLY REFRESHES CONSOLE                  */
/* ========================================================= */

static void console_refresh_task(void *arg)
{
    // Wait a bit for UI to be fully created
    vTaskDelay(pdMS_TO_TICKS(500));
    while (1)
    {
        if (current_app == APP_CONSOLE && lvgl_port_lock(0))
        {
            refresh_console_text();
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ========================================================= */
/* MAIN                                                      */
/* ========================================================= */

void app_main(void)
{
    bsp_display_start();
    bsp_display_backlight_on();

    // Redirect ESP_LOG to our ring buffer
    esp_log_set_vprintf(custom_log_vprintf);

    // Create UI under LVGL lock
    if (lvgl_port_lock(0))
    {
        create_ui();
        lvgl_port_unlock();
    }
    else
    {
        ESP_LOGE(TAG, "Failed to lock LVGL mutex");
        return;
    }

    // Start console refresh task (priority 3)
    xTaskCreate(console_refresh_task, "refresh", 4096, NULL, 3, NULL);

    // Start background tasks
    xTaskCreate(console_task, "console", 4096, NULL, 4, NULL);
    xTaskCreate(music_task, "music", 4096, NULL, 4, NULL);
    xTaskCreate(nes_task, "nes", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "System started");
}