#include "nvs_data.h"
static const char *TAG = "NVS_DATA";
nvs_handle_t nvs_data_handle;   
typedef union
{
    float f;
    uint32_t d;
}FTOD;
SYS_CONFIG sys_config;

void sys_data_init()
{
    
    uint8_t mac[6];
    esp_base_mac_addr_get(mac);
    for (uint8_t i = 0; i < 3; i++)
    {
       sprintf(sys_config.mac+i*2,"%02X",mac[i+3]);
    }
    sys_config.app_desc = esp_app_get_description();
    sys_config.set_light = nvs_get_u8_data(SET_NVS_LIGHT);
    sys_config.set_lock = nvs_get_u8_data(SET_NVS_LOCK); //5min
    sys_config.set_sleep = nvs_get_u8_data(SET_NVS_SLEEP); //10min
    sys_config.set_rotation = nvs_get_u8_data(SET_NVS_ROTATION);
    sys_config.set_wifi = nvs_get_u8_data(SET_NVS_WIFI);
    sys_config.set_frist_screen = nvs_get_u8_data(SET_NVS_FIRST_SCREEN);
    sys_config.hid_sys_index = nvs_get_u8_data(NVS_SYS_HID_INDEX);
    sys_config.hid_csm_index = nvs_get_u8_data(NVS_CSM_HID_INDEX);
    sys_config.foc_angle = nvs_get_float_data(NVS_FOC_ELECTRIC_ANGLE);
    
    // 读取自定义HID配置，如果不存在或大小不匹配则清零
    if (nvs_get_blob_data(NVS_CUSTOM_HID_DATA, &sys_config.custom_hid, sizeof(CUSTOM_HID_CONFIG)) != sizeof(CUSTOM_HID_CONFIG)) {
        ESP_LOGW(TAG, "Custom HID config not found or invalid size, using defaults");
        memset(&sys_config.custom_hid, 0, sizeof(CUSTOM_HID_CONFIG));
    }
}
static void nvs_default_data_set()
{
    nvs_set_u8_data("flag",1);
    nvs_set_u8_data(SET_NVS_LIGHT,50);
    nvs_set_u8_data(SET_NVS_LOCK,3); //5min
    nvs_set_u8_data(SET_NVS_SLEEP,3); //10min
    nvs_set_u8_data(SET_NVS_ROTATION,0);
    nvs_set_u8_data(SET_NVS_WIFI,0);
    nvs_set_u8_data(SET_NVS_FIRST_SCREEN,0);
    nvs_set_u8_data(NVS_SYS_HID_INDEX,0);
    nvs_set_u8_data(NVS_CSM_HID_INDEX,0);
    nvs_set_float_data(NVS_FOC_ELECTRIC_ANGLE,0);
    
    // 设置默认自定义HID配置 (全空)
    CUSTOM_HID_CONFIG default_hid = {0};
    nvs_set_blob_data(NVS_CUSTOM_HID_DATA, &default_hid, sizeof(CUSTOM_HID_CONFIG));
    
    ESP_LOGI(TAG,"nvs_default_data_set");
}
void nvs_data_init()
{
    esp_err_t ret;
    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    if(nvs_get_u8_data("flag") != 1)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        ESP_ERROR_CHECK(ret);
        nvs_default_data_set();
    }
    else
    {
        sys_data_init();
    }
}
void nvs_set_u8_data(const char* key_name,uint8_t value)
{
    esp_err_t ret;
    ret = nvs_open("main", NVS_READWRITE, &nvs_data_handle);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "opening NVS Error (%s)!\n", esp_err_to_name(ret));
    } 
    else
    {
        ret = nvs_set_u8(nvs_data_handle, key_name, value);
        if(ret!=ESP_OK) ESP_LOGE(TAG, "%s set Error",key_name);
    }
    nvs_close(nvs_data_handle);
}
uint8_t nvs_get_u8_data(const char* key_name)
{
    uint8_t value = 0;
    esp_err_t ret;
    ret = nvs_open("main", NVS_READWRITE, &nvs_data_handle);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "opening NVS Error (%s)!\n", esp_err_to_name(ret));
    } 
    else
    {
        ret = nvs_get_u8(nvs_data_handle, key_name, &value);
        if(ret!=ESP_OK) ESP_LOGE(TAG, "%s get Error",key_name);
    }
    nvs_close(nvs_data_handle);
    return value;
}
void nvs_set_float_data(const char* key_name,float value)
{
    FTOD da;
    da.f = value;
    esp_err_t ret;
    ret = nvs_open("main", NVS_READWRITE, &nvs_data_handle);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "opening NVS Error (%s)!\n", esp_err_to_name(ret));
    } 
    else
    {
        ret = nvs_set_u32(nvs_data_handle, key_name, da.d);
        if(ret!=ESP_OK) ESP_LOGE(TAG, "%s set Error",key_name);
    }
    nvs_close(nvs_data_handle);
}
float nvs_get_float_data(const char* key_name)
{
    FTOD da;
    esp_err_t ret;
    ret = nvs_open("main", NVS_READWRITE, &nvs_data_handle);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "opening NVS Error (%s)!\n", esp_err_to_name(ret));
    } 
    else
    {
        da.d = 0;
        ret = nvs_get_u32(nvs_data_handle, key_name, &da.d);
        if(ret!=ESP_OK) ESP_LOGE(TAG, "%s get Error",key_name);
    }
    nvs_close(nvs_data_handle);
    return da.f;
}

void nvs_set_blob_data(const char* key_name, void* value, size_t length)
{
    esp_err_t ret;
    ret = nvs_open("main", NVS_READWRITE, &nvs_data_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "opening NVS Error (%s)!\\n", esp_err_to_name(ret));
    } else {
        ret = nvs_set_blob(nvs_data_handle, key_name, value, length);
        if(ret != ESP_OK) ESP_LOGE(TAG, "%s set blob Error", key_name);
        nvs_commit(nvs_data_handle);
    }
    nvs_close(nvs_data_handle);
}

size_t nvs_get_blob_data(const char* key_name, void* value, size_t length)
{
    esp_err_t ret;
    size_t required_size = 0;
    ret = nvs_open("main", NVS_READWRITE, &nvs_data_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "opening NVS Error (%s)!\\n", esp_err_to_name(ret));
        return 0;
    }
    
    // 获取大小
    ret = nvs_get_blob(nvs_data_handle, key_name, NULL, &required_size);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "%s get blob size Error", key_name);
        nvs_close(nvs_data_handle);
        return 0;
    }
    
    if (required_size == 0) {
        nvs_close(nvs_data_handle);
        return 0;
    }
    
    if (length < required_size) {
        ESP_LOGW(TAG, "%s blob truncated", key_name);
        // 如果缓冲区太小，只读部分（虽然nvs_get_blob不支持partial read，但这里为了逻辑完整）
        // 实测 NVS blob 必须一次读完
    }
    
    ret = nvs_get_blob(nvs_data_handle, key_name, value, &required_size);
    if(ret != ESP_OK) ESP_LOGE(TAG, "%s get blob Error", key_name);
    
    nvs_close(nvs_data_handle);
    return required_size;
}
