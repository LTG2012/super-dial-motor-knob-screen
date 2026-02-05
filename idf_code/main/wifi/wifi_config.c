#include "wifi_config.h"
#include "../sntp_time/sntp_time.h"
#include "../spiffs_init/spiffs_init.h"
#include "cJSON.h"
#include "nvs_data/nvs_data.h"
#include "web_server.h"
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
            esp_netif_deinit();
            stop_webserver(server);
            server = NULL;
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
        server = start_webserver();
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
    stop_webserver(server);
    server = NULL;
}
void get_wifi_current_status(char *src)
{
    strcpy(src, wifi_current_src);
}