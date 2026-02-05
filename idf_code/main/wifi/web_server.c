#include "web_server.h"
#include "web_html.h"
#include "../sntp_time/sntp_time.h"
#include "../spiffs_init/spiffs_init.h"
#include "cJSON.h"
#include "nvs_data/nvs_data.h"
#include "../usb_device/usb_device.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"

static const char *TAG = "WEB_SERVER";

// HTTP GET Handler - 返回配置页面
static esp_err_t root_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serve config page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WEB_CONFIG_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// API: 获取配置信息
static esp_err_t api_config_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API: Get config");
    
    size_t total = 0, used = 0;
    spiffs_get_info(&total, &used);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", sys_config.mac);
    if(sys_config.app_desc) {
        cJSON_AddStringToObject(root, "fw_version", sys_config.app_desc->version);
    } else {
        cJSON_AddStringToObject(root, "fw_version", "unknown");
    }
    
    char storage_str[32];
    snprintf(storage_str, sizeof(storage_str), "%d/%d KB", (int)(used/1024), (int)(total/1024));
    cJSON_AddStringToObject(root, "storage", storage_str);
    cJSON_AddNumberToObject(root, "bg_index", sys_config.bg_index);
    
    cJSON *slots = cJSON_CreateArray();
    for(int i=0; i<CUSTOM_HID_SLOT_NUM; i++) {
        cJSON *item = cJSON_CreateObject();
        HID_CONFIG_ITEM *cfg = &sys_config.custom_hid.items[i];
        
        cJSON_AddBoolToObject(item, "enabled", cfg->enabled ? true : false);
        cJSON_AddStringToObject(item, "name", cfg->name);
        cJSON_AddNumberToObject(item, "hid_type", cfg->hid_type);
        
        // 转换按键码为字符串 (简化处理，目前只显示占位符或需要反解析)
        cJSON_AddStringToObject(item, "cw_key", ""); 
        cJSON_AddStringToObject(item, "ccw_key", "");
        cJSON_AddStringToObject(item, "click_key", "");
        
        cJSON_AddItemToArray(slots, item);
    }
    cJSON_AddItemToObject(root, "hid_slots", slots);
    
    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    
    free((void*)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// 辅助函数：解析按键字符串
// 格式: "CTRL+ALT+A", "SHIFT+F1", "ENTER" 等
static void parse_key_string(const char *str, uint8_t *cw)
{
    memset(cw, 0, 6);
    if (!str || strlen(str) == 0) return;
    
    char temp[64];
    strncpy(temp, str, sizeof(temp)-1);
    temp[sizeof(temp)-1] = 0;
    
    uint8_t modifiers = 0;
    uint8_t keycode = 0;
    
    char *token = strtok(temp, "+");
    while (token != NULL) {
        // 转换大写
        for(int i=0; token[i]; i++) {
            if(token[i] >= 'a' && token[i] <= 'z') token[i] -= 32;
        }
        
        // Modifiers
        if (strcmp(token, "CTRL") == 0 || strcmp(token, "CONTROL") == 0) modifiers |= KEYBOARD_MODIFIER_LEFTCTRL;
        else if (strcmp(token, "SHIFT") == 0) modifiers |= KEYBOARD_MODIFIER_LEFTSHIFT;
        else if (strcmp(token, "ALT") == 0) modifiers |= KEYBOARD_MODIFIER_LEFTALT;
        else if (strcmp(token, "GUI") == 0 || strcmp(token, "WIN") == 0 || strcmp(token, "CMD") == 0) modifiers |= KEYBOARD_MODIFIER_LEFTGUI;
        else if (strcmp(token, "VOL+") == 0 || strcmp(token, "VOLUMEUP") == 0) { cw[0] = 0xE9; cw[1] = 0; return; }
        else if (strcmp(token, "VOL-") == 0 || strcmp(token, "VOLUMEDOWN") == 0) { cw[0] = 0xEA; cw[1] = 0; return; }
        else if (strcmp(token, "MUTE") == 0) { cw[0] = 0xE2; cw[1] = 0; return; }
        else if (strcmp(token, "PLAY") == 0 || strcmp(token, "PAUSE") == 0) { cw[0] = 0xCD; cw[1] = 0; return; }
        else if (strcmp(token, "NEXT") == 0) { cw[0] = 0xB5; cw[1] = 0; return; }
        else if (strcmp(token, "PREV") == 0) { cw[0] = 0xB6; cw[1] = 0; return; }
        else {
            // Keycode mapping (Simple implementation for common keys)
            if (strlen(token) == 1) {
                char c = token[0];
                if (c >= 'A' && c <= 'Z') keycode = HID_KEY_A + (c - 'A');
                else if (c >= '1' && c <= '9') keycode = HID_KEY_1 + (c - '1');
                else if (c == '0') keycode = HID_KEY_0;
            } else {
                if (strcmp(token, "ENTER") == 0) keycode = HID_KEY_ENTER;
                else if (strcmp(token, "ESC") == 0 || strcmp(token, "ESCAPE") == 0) keycode = HID_KEY_ESCAPE;
                else if (strcmp(token, "BACKSPACE") == 0) keycode = HID_KEY_BACKSPACE;
                else if (strcmp(token, "TAB") == 0) keycode = HID_KEY_TAB;
                else if (strcmp(token, "SPACE") == 0) keycode = HID_KEY_SPACE;
                else if (strcmp(token, "LEFT") == 0) keycode = HID_KEY_ARROW_LEFT;
                else if (strcmp(token, "RIGHT") == 0) keycode = HID_KEY_ARROW_RIGHT;
                else if (strcmp(token, "UP") == 0) keycode = HID_KEY_ARROW_UP;
                else if (strcmp(token, "DOWN") == 0) keycode = HID_KEY_ARROW_DOWN;
                else if (strncmp(token, "F", 1) == 0 && strlen(token) > 1) {
                    int f_num = atoi(token + 1);
                    if (f_num >= 1 && f_num <= 12) keycode = HID_KEY_F1 + (f_num - 1);
                }
            }
        }
        token = strtok(NULL, "+");
    }
    
    cw[0] = modifiers;
    cw[1] = keycode;
}

// API: 保存配置
static esp_err_t api_config_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API: Save config");
    
    char buf[2048]; // 增加缓冲区以容纳完整的JSON
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
        return ESP_FAIL;
    }
    buf[received] = '\0';
    
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *slots = cJSON_GetObjectItem(root, "hid_slots");
    if (cJSON_IsArray(slots)) {
        int count = cJSON_GetArraySize(slots);
        if (count > CUSTOM_HID_SLOT_NUM) count = CUSTOM_HID_SLOT_NUM;
        
        for (int i = 0; i < count; i++) {
            cJSON *item = cJSON_GetArrayItem(slots, i);
            HID_CONFIG_ITEM *cfg = &sys_config.custom_hid.items[i];
            
            cJSON *enabled = cJSON_GetObjectItem(item, "enabled");
            if(enabled) cfg->enabled = cJSON_IsTrue(enabled) ? 1 : 0;
            
            cJSON *name = cJSON_GetObjectItem(item, "name");
            if(cJSON_IsString(name)) {
                strncpy(cfg->name, name->valuestring, sizeof(cfg->name)-1);
            }
            
            cJSON *hid_type = cJSON_GetObjectItem(item, "hid_type");
            if(cJSON_IsNumber(hid_type)) {
                cfg->hid_type = (uint8_t)hid_type->valueint;
            }
            
            
            // 解析 key strings
            cJSON *cw_key = cJSON_GetObjectItem(item, "cw_key");
            if (cJSON_IsString(cw_key)) {
                parse_key_string(cw_key->valuestring, cfg->cw);
            }
            
            cJSON *ccw_key = cJSON_GetObjectItem(item, "ccw_key");
            if (cJSON_IsString(ccw_key)) {
                parse_key_string(ccw_key->valuestring, cfg->ccw);
            }
            
            cJSON *click_key = cJSON_GetObjectItem(item, "click_key");
            if (cJSON_IsString(click_key)) {
                parse_key_string(click_key->valuestring, cfg->click);
            }
        }
        
        // 保存到NVS
        nvs_set_blob_data(NVS_CUSTOM_HID_DATA, &sys_config.custom_hid, sizeof(CUSTOM_HID_CONFIG));
    }
    
    cJSON *bg_idx = cJSON_GetObjectItem(root, "bg_index");
    if(cJSON_IsNumber(bg_idx)) {
        sys_config.bg_index = (uint8_t)bg_idx->valueint;
        nvs_set_u8_data(NVS_BG_INDEX, sys_config.bg_index);
    }
    
    cJSON_Delete(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"配置已保存\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// API: 获取当前时间
static esp_err_t api_time_get_handler(httpd_req_t *req)
{
    char time_str[32];
    if (sntp_time_get_time(time_str, sizeof(time_str)) != ESP_OK) {
        strcpy(time_str, "--:--:--");
    }
    
    char response[64];
    snprintf(response, sizeof(response), "{\"time\":\"%s\",\"synced\":%s}",
        time_str, sntp_time_is_synced() ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// API: 上传图片
static esp_err_t api_upload_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API: Upload image, size=%d", req->content_len);
    
    if (req->content_len > 150 * 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File too large (max 150KB)");
        return ESP_FAIL;
    }
    
    // 获取 X-File-Name 头
    char file_name[64];
    if (httpd_req_get_hdr_value_str(req, "X-File-Name", file_name, sizeof(file_name)) != ESP_OK) {
        strcpy(file_name, "bg_custom.bin");
    }
    
    // 简单校验
    if (strstr(file_name, "..") || strchr(file_name, '/')) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid filename");
        return ESP_FAIL;
    }

    char filepath[128];
    snprintf(filepath, sizeof(filepath), "/spiffs/%s", file_name);
    
    // 打开文件准备写入
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }
    
    char buf[1024];
    int received;
    int total_received = 0;
    
    while ((received = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, received, f);
        total_received += received;
    }
    fclose(f);
    
    ESP_LOGI(TAG, "Image saved, total bytes: %d", total_received);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"图片上传成功\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// API: 保存WiFi配置
static esp_err_t api_wifi_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API: Save WiFi config");
    
    char buf[256];
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
        return ESP_FAIL;
    }
    buf[received] = '\0';
    
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *ssid_item = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass_item = cJSON_GetObjectItem(root, "password");
    
    if (cJSON_IsString(ssid_item) && (cJSON_IsString(pass_item) || cJSON_IsNull(pass_item))) {
        char *ssid = ssid_item->valuestring;
        char *pass = cJSON_IsString(pass_item) ? pass_item->valuestring : "";
        
        ESP_LOGI(TAG, "Setting WiFi: SSID=%s", ssid);
        
        wifi_config_t wifi_config = {0};
        // 先获取当前配置（主要为了其它字段，虽然这里重新设置STA）
        esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
        
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = (strlen(pass) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        
        // 立即尝试连接
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_connect());
        
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"success\":true,\"message\":\"WiFi配置已保存，正在尝试连接...\"}", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid or password");
    }
    
    cJSON_Delete(root);
    return ESP_OK;    
}

// API: OTA 固件升级
static esp_err_t api_update_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "API: Firmware Update, size=%d", req->content_len);
    
    if (req->content_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }
    
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No partition found for OTA");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No partition for OTA");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
             update_partition->subtype, update_partition->address);
             
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Begin Failed");
        return ESP_FAIL;
    }
    
    char *buf = malloc(4096);
    if (!buf) {
        esp_ota_end(update_handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
        return ESP_FAIL;
    }
    
    int received;
    int total_received = 0;
    while ((received = httpd_req_recv(req, buf, 4096)) > 0) {
        err = esp_ota_write(update_handle, buf, received);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            free(buf);
            esp_ota_end(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Write Failed");
            return ESP_FAIL;
        }
        total_received += received;
    }
    free(buf);
    
    if (received < 0) {
        esp_ota_end(update_handle);
        return ESP_FAIL;
    }
    
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA End Failed");
        return ESP_FAIL;
    }
    
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set Boot Partition Failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "OTA Success! Rebooting...");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true,\"message\":\"升级成功，正在重启...\"}", HTTPD_RESP_USE_STRLEN);
    
    // 延迟重启
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

static const httpd_uri_t api_config_get = {
    .uri = "/api/config",
    .method = HTTP_GET,
    .handler = api_config_get_handler,
};

static const httpd_uri_t api_config_post = {
    .uri = "/api/config",
    .method = HTTP_POST,
    .handler = api_config_post_handler,
};

static const httpd_uri_t api_time = {
    .uri = "/api/time",
    .method = HTTP_GET,
    .handler = api_time_get_handler,
};

static const httpd_uri_t api_upload = {
    .uri = "/api/upload",
    .method = HTTP_POST,
    .handler = api_upload_post_handler,
};

static const httpd_uri_t api_wifi = {
    .uri = "/api/wifi",
    .method = HTTP_POST,
    .handler = api_wifi_post_handler,
};

static const httpd_uri_t api_update = {
    .uri = "/api/update",
    .method = HTTP_POST,
    .handler = api_update_post_handler,
};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // config.stack_size = 8192 * 4; // 32KB
    config.max_open_sockets = 10;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &api_config_get);
        httpd_register_uri_handler(server, &api_config_post);
        httpd_register_uri_handler(server, &api_time);
        httpd_register_uri_handler(server, &api_upload);
        httpd_register_uri_handler(server, &api_wifi);
        httpd_register_uri_handler(server, &api_update);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}

void stop_webserver(httpd_handle_t server)
{
    if (server)
    {
        httpd_stop(server);
    }
}
