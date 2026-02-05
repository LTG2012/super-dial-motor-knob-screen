#include "../ui.h"
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "../../sntp_time/sntp_time.h"

static const char *TAG = "SCREEN_CLOCK";
static lv_timer_t *clock_timer;
static lv_obj_t *label_time;
static lv_obj_t *label_date;
static lv_obj_t *label_weekday;
static lv_obj_t *label_bat;
static lv_obj_t *label_steps;
static lv_obj_t *label_bpm;
static lv_obj_t *label_temp;
static lv_obj_t *img_astro_proxy; // Rotating dial as astronaut proxy
static lv_group_t *group;

// Styles
static lv_style_t style_time;
static lv_style_t style_data;
static lv_style_t style_unit;

static void style_init(void) {
    lv_style_init(&style_time);
    lv_style_set_text_font(&style_time, &ui_font_SmileySansOblique20); 
    lv_style_set_text_color(&style_time, lv_color_white());

    lv_style_init(&style_data);
    lv_style_set_text_font(&style_data, &ui_font_SmileySansOblique16);
    lv_style_set_text_color(&style_data, lv_color_hex(0x00FF00)); // Matrix Green or Cyan

    lv_style_init(&style_unit);
    lv_style_set_text_font(&style_unit, &ui_font_SmileySansOblique16);
    lv_style_set_text_color(&style_unit, lv_color_hex(0x888888));
}

static void update_time_cb(lv_timer_t * timer) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[32];

    // 1. Update Time (Large) - HH:MM:SS
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    lv_label_set_text(label_time, buf);

    // 2. Update Date - MM-DD WWW
    // Simplified formatting
    const char *weeks[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    int wday = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday < 7) ? timeinfo.tm_wday : 0;
    snprintf(buf, sizeof(buf), "%02d-%02d %s", timeinfo.tm_mon + 1, timeinfo.tm_mday, weeks[wday]);
    lv_label_set_text(label_date, buf);

    // 3. Update Battery (Real)
    uint8_t bat_val = bat_val_get();
    snprintf(buf, sizeof(buf), "BAT %d%%", bat_val);
    lv_label_set_text(label_bat, buf);
    // Color battery based on level
    if(bat_val < 20) lv_obj_set_style_text_color(label_bat, lv_color_hex(0xFF0000), 0);
    else lv_obj_set_style_text_color(label_bat, lv_color_hex(0x00FF00), 0);

    // 4. Animate "Astronaut" (Rotate image)
    // 10 degrees per second (since timer is 1000ms... wait, maybe too choppy. 
    // Ideally use animation, but timer is simplest).
    uint16_t angle = lv_img_get_angle(img_astro_proxy);
    lv_img_set_angle(img_astro_proxy, angle + 100); // +10.0 degrees
}

// Event handler
void ui_Screen_Clock_event(uint8_t state)
{
    switch (state)
    {
        case DIAL_STA_CLICK:
        case DIAL_STA_DOUBLE_CLICK:
             _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_Screen1_screen_init);
            break;
        default: break;
    }
}

static void create_data_label(lv_obj_t *parent, lv_obj_t **var, const char *def_text, int x, int y, lv_align_t align) {
    *var = lv_label_create(parent);
    lv_obj_add_style(*var, &style_data, 0);
    lv_label_set_text(*var, def_text);
    lv_obj_align(*var, align, x, y);
}

void scr_Screen_Clock_loaded_cb(lv_event_t * e) {
    clock_timer = lv_timer_create(update_time_cb, 500, NULL); // Update every 500ms
    update_time_cb(clock_timer);
    lv_indev_set_group(encoder_indev, group);
}

void scr_Screen_Clock_unloaded_cb(lv_event_t * e) {
    if(clock_timer) { lv_timer_del(clock_timer); clock_timer = NULL; }
    if(group) { lv_group_del(group); group = NULL; }
}

void ui_Screen_Clock_screen_init(void) {
    ui_Screen_Clock = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Clock, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_Clock, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_Screen_Clock, LV_OPA_COVER, 0);

    lv_obj_add_event_cb(ui_Screen_Clock, scr_Screen_Clock_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(ui_Screen_Clock, scr_Screen_Clock_unloaded_cb, LV_EVENT_SCREEN_UNLOADED, &ui_Screen_Clock);

    style_init();

    // --- Layout ---
    // 1. Center Image (Astronaut Proxy)
    img_astro_proxy = lv_img_create(ui_Screen_Clock);
    lv_img_set_src(img_astro_proxy, &ui_img_dial_png); // Use dial icon as proxy
    lv_img_set_zoom(img_astro_proxy, 180); // Scale down slightly
    lv_obj_align(img_astro_proxy, LV_ALIGN_CENTER, 0, -20);
    
    // 2. Time (Big, Bottom Center)
    label_time = lv_label_create(ui_Screen_Clock);
    lv_obj_add_style(label_time, &style_time, 0);
    lv_obj_set_style_transform_zoom(label_time, 400, 0); // Scale up 1.5x
    lv_label_set_text(label_time, "00:00:00");
    lv_obj_align(label_time, LV_ALIGN_CENTER, 0, 50);

    // 3. Date (Below Time)
    label_date = lv_label_create(ui_Screen_Clock);
    lv_obj_add_style(label_date, &style_data, 0);
    lv_label_set_text(label_date, "01-01 MON");
    lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 80);

    // 4. Corners (Mock Data)
    // Top Left: Temp
    create_data_label(ui_Screen_Clock, &label_temp, "24°C", 10, 10, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFFA500), 0); // Orange

    // Top Right: Battery
    create_data_label(ui_Screen_Clock, &label_bat, "BAT --%", -10, 10, LV_ALIGN_TOP_RIGHT);
    
    // Bottom Left: Heart Rate
    create_data_label(ui_Screen_Clock, &label_bpm, "88 BPM", 10, -30, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_style_text_color(label_bpm, lv_color_hex(0xFF0000), 0); // Red
    
    // Bottom Right: Steps
    create_data_label(ui_Screen_Clock, &label_steps, "5624 steps", -10, -30, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_style_text_color(label_steps, lv_color_hex(0x00FFFF), 0); // Cyan

    // Lines (Decoration)
    lv_obj_t *line1 = lv_line_create(ui_Screen_Clock);
    static lv_point_t line_points[] = { {0, 110}, {240, 110} }; // Horizontal Line
    lv_line_set_points(line1, line_points, 2);
    lv_obj_set_style_line_color(line1, lv_color_hex(0x444444), 0);
    lv_obj_set_style_line_width(line1, 1, 0);
    lv_obj_align(line1, LV_ALIGN_CENTER, 0, 20);

    // Group for encoder
    group = lv_group_create();
    lv_obj_t * btn = lv_btn_create(ui_Screen_Clock);
    lv_obj_set_size(btn, 240, 240);
    lv_obj_center(btn);
    lv_obj_set_style_bg_opa(btn, 0, 0);
    lv_group_add_obj(group, btn);
    lv_group_focus_obj(btn);
}
