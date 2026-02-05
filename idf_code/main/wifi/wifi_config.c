#include "wifi_config.h"
#include "web_html.h"
#include "../sntp_time/sntp_time.h"
#include "../spiffs_init/spiffs_init.h"
#include "cJSON.h"
#include "nvs_data/nvs_data.h"
#include "../usb_device/usb_device.h"
static bool g_wifi_sta_inited = false;
static bool g_wifi_ap_inited = false;
static bool g_wifi_sta_ap_state = false;
static char wifi_current_src[100] = "";
// SemaphoreHandle_t ap_sem;
esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;

#define SCRATCH_BUFSIZE 8192
static const char *TAG = "TAG";
#define EXAMPLE_ESP_MAXIMUM_RETRY 10
#define PRIFIX_NAME "SuperDial"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
esp_netif_t *esp_netif;
httpd_handle_t server = NULL;

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS ""
#define EXAMPLE_MAX_STA_CONN CONFIG_LWIP_MAX_SOCKETS

#define EXAMPLE_ESP_MAXIMUM_RETRY 5
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static void wifi_deinit(); // 关闭wifi热点
static void wifi_ap_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_station_task(void *pvParameters)
{
    uint32_t result = 0;
    while (1)
    {
        // result = xSemaphoreTake(ap_sem, portMAX_DELAY);
        if (result == pdPASS)
        {
            esp_restart();
            esp_wifi_stop();
            esp_event_handler_unregister(WIFI_EVENT,
                                         ESP_EVENT_ANY_ID,
                                         &wifi_ap_event_handler);
            esp_netif_destroy_default_wifi(esp_netif);
            esp_event_loop_delete_default();
            esp_wifi_deinit();
            esp_netif_deinit();
            httpd_stop(server);
            wifi_init_sta();
            ESP_LOGI(TAG, "wifi station inited...");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

//---------------------wifi_sta-----------------------//
static int s_retry_num = 0;

static void sta_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
        // ESP_LOGE(TAG, "Disconnect reason : %d", disconnected->reason);
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            char src[6] = "......";
            char src_num[6] = "";
            strncpy(src_num, src, s_retry_num);
            sprintf(wifi_current_src, "retry to connect %s", src_num);
            ESP_LOGI(TAG, "retry to connect to the AP ,s_retry_num:%d", s_retry_num);
        }
        else
        {
            // xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            // wifi_deinit();
            sprintf(wifi_current_src, "connect to the AP fail");
            ESP_LOGI(TAG, "connect to the AP fail");
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_BEACON_TIMEOUT)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_BEACON_TIMEOUT");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        sprintf(wifi_current_src, "IP:%d.%d.%d.%d", IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        g_wifi_sta_ap_state = 0;
        
        // WiFi连接成功后自动启动SNTP时间同步
        ESP_LOGI(TAG, "Starting SNTP time synchronization...");
        sntp_time_init();
        
        // xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
void wifi_init_sta()
{
    wifi_config_t wifi_config = {0};
    // 尝试从NVS获取WiFi配置
    esp_err_t err = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
    
    // 如果获取失败或者SSID为空，则使用默认值或返回
    if (err != ESP_OK || strlen((char *)wifi_config.sta.ssid) == 0)
    {
        ESP_LOGI(TAG, "No saved wifi config found");
        sprintf(wifi_current_src, "No WiFi Config");
        
        // 确保初始化WiFi驱动，即使没有配置，以便后续可以设置
        // 但如果不连接，在这里返回即可，等待用户通过网页配置
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        return;
    }
    
    ESP_LOGI(TAG, "Found saved wifi config: %s", wifi_config.sta.ssid);
    sprintf(wifi_current_src, "Connect: %s", wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_retry_num = 0;

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    esp_wifi_connect();
}
static void wifi_deinit() // 关闭wifi热点
{
    if (g_wifi_ap_inited == 0 && g_wifi_sta_inited == 0)
    {
        return;
    }
    if (g_wifi_sta_inited)
    {
        /* The event will not be processed after unregister */
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        g_wifi_sta_inited = 0;
        s_retry_num = 0;
    }
    if (g_wifi_ap_inited)
    {
        esp_event_handler_unregister(WIFI_EVENT,
                                     ESP_EVENT_ANY_ID,
                                     &wifi_ap_event_handler);
        g_wifi_ap_inited = 0;
    }
    esp_wifi_stop();
    esp_netif_destroy_default_wifi(esp_netif);
    // esp_event_loop_delete_default();
    esp_wifi_deinit();
    // esp_netif_deinit();
    // httpd_stop(server);
    ESP_LOGI(TAG, "wifi deinit...");
}
void wifi_init_softap(void)
{
    char wifi_ap_name[32];
    sprintf(wifi_ap_name, "%s-%s", PRIFIX_NAME, sys_config.mac);

    wifi_config_t wifi_config = {
        .ap = {
            // .ssid = wifi_ap_name,
            .ssid_len = strlen(wifi_ap_name),
            .password = "",
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    memcpy(wifi_config.ap.ssid, wifi_ap_name, sizeof(wifi_config.ap.ssid));
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); // AP+STA
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    sprintf(wifi_current_src, "Wifi:%16s  \nIP:%s", wifi_ap_name, ip_addr);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             wifi_ap_name, EXAMPLE_ESP_WIFI_PASS);
    g_wifi_sta_ap_state = 1;
}

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
    
    cJSON *slots = cJSON_CreateArray();
    for(int i=0; i<CUSTOM_HID_SLOT_NUM; i++) {
        cJSON *item = cJSON_CreateObject();
        HID_CONFIG_ITEM *cfg = &sys_config.custom_hid.items[i];
        
        cJSON_AddBoolToObject(item, "enabled", cfg->enabled ? true : false);
        cJSON_AddStringToObject(item, "name", cfg->name);
        cJSON_AddNumberToObject(item, "hid_type", cfg->hid_type);
        
        // 转换按键码为字符串 (简化处理，目前只显示占位符或需要反解析)
        // 这里的逻辑需要根据web_html.h的约定，Web端发送的是字符串，但NVS存的是u8[6]
        // 暂时简单处理：如果全是0，返回空字符串
        // TODO: 实现 keycode -> string 的转换，或者直接存储字符串
        // 为了兼容当前演示，先返回空字符串，等待后续完善映射逻辑
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
    
    // 打开文件准备写入
    FILE *f = fopen("/spiffs/bg_custom.bin", "wb");
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
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}
void wifi_init(uint8_t mode)
{
    static bool wifi_init_flag = false;
    if (wifi_init_flag == false)
    {
        wifi_init_flag = true;
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &sta_event_handler,
                                                            NULL,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &sta_event_handler,
                                                            NULL,
                                                            &instance_got_ip));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL));
        start_webserver();
    }

    if (mode == 1) // STA
    {
        wifi_init_sta();
    }
    else if (mode == 2) // AP
    {
        wifi_init_softap();
    }
    else
    {
        ESP_LOGE(TAG, "wifi mode error");
    }
}
void wifi_stop(void)
{
    esp_wifi_stop();
    // httpd_stop(server);
}
void get_wifi_current_status(char *src)
{
    strcpy(src, wifi_current_src);
}