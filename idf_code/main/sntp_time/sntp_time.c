#include "sntp_time.h"
#include <string.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SNTP_TIME";
static bool s_time_synced = false;

// 时间同步回调
static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized!");
    s_time_synced = true;
}

void sntp_time_init(void)
{
    ESP_LOGI(TAG, "Initializing SNTP...");
    
    // 设置时区为中国标准时间 (UTC+8)
    setenv("TZ", "CST-8", 1);
    tzset();
    
    // 配置SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    
    // 设置NTP服务器（主服务器 + 备用服务器）
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "cn.ntp.org.cn");
    esp_sntp_setservername(2, "ntp.aliyun.com");
    
    // 设置同步回调
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    
    // 启动SNTP
    esp_sntp_init();
    
    ESP_LOGI(TAG, "SNTP initialized, waiting for time sync...");
}

void sntp_time_stop(void)
{
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
        s_time_synced = false;
        ESP_LOGI(TAG, "SNTP stopped");
    }
}

bool sntp_time_is_synced(void)
{
    return s_time_synced;
}

esp_err_t sntp_time_get(struct tm *timeinfo)
{
    if (timeinfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    
    // 检查年份是否合理（2024年以后视为有效）
    if (timeinfo->tm_year < (2024 - 1900)) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t sntp_time_get_str(char *buf, size_t buf_len, const char *format)
{
    if (buf == NULL || buf_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct tm timeinfo;
    if (sntp_time_get(&timeinfo) != ESP_OK) {
        snprintf(buf, buf_len, "Time not synced");
        return ESP_FAIL;
    }
    
    const char *fmt = format ? format : "%Y-%m-%d %H:%M:%S";
    strftime(buf, buf_len, fmt, &timeinfo);
    
    return ESP_OK;
}

esp_err_t sntp_time_get_date(char *buf, size_t buf_len)
{
    return sntp_time_get_str(buf, buf_len, "%Y-%m-%d");
}

esp_err_t sntp_time_get_time(char *buf, size_t buf_len)
{
    return sntp_time_get_str(buf, buf_len, "%H:%M:%S");
}
