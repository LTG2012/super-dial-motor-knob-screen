#include <string.h>

#include "nvs_data.h"
#include "dial.h"
#include "display.h"
#include "wifi_config.h"
#include "dial_power.h"
#include "usb_device.h"

QueueHandle_t HID_Queue = NULL;

static const esp_app_desc_t s_sim_app_desc = {
    .date = "sim"
};

SYS_CONFIG sys_config = {
    .set_light = 50,
    .set_lock = 3,
    .set_sleep = 3,
    .set_rotation = 0,
    .set_wifi = 0,
    .set_frist_screen = 0,
    .hid_sys_index = 0,
    .hid_csm_index = 0,
    .mac = { '0','0','0','0','0','0' },
    .foc_angle = 0.0f,
    .app_desc = &s_sim_app_desc,
};

void nvs_data_init(void) {}

void nvs_set_u8_data(const char* key_name, uint8_t value)
{
    (void)key_name;
    (void)value;
}

uint8_t nvs_get_u8_data(const char* key_name)
{
    (void)key_name;
    return 0;
}

void nvs_set_float_data(const char* key_name, float value)
{
    (void)key_name;
    (void)value;
}

float nvs_get_float_data(const char* key_name)
{
    (void)key_name;
    return 0.0f;
}

void foc_knob_set_param(foc_knob_param_t param)
{
    (void)param;
}

void set_screen_light(uint8_t duty)
{
    (void)duty;
}

void set_screen_rotation(uint8_t val)
{
    (void)val;
}

void wifi_init(uint8_t mode)
{
    (void)mode;
}

void wifi_stop(void) {}

void get_wifi_current_status(char *src)
{
    if(src) {
        strcpy(src, "SIM");
    }
}

void adc_read_init(void) {}
void power_gpio_init(void) {}
void power_on(void) {}
void power_off(void) {}
void bat_power_get(void) {}

uint8_t bat_val_get(void)
{
    return 75;
}

uint32_t bat_adc_get(void)
{
    return 0;
}

void set_new_lock_time(uint8_t val)
{
    (void)val;
}

void set_new_sleep_time(uint8_t val)
{
    (void)val;
}
