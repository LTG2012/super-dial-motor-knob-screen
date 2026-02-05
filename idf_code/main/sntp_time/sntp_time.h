#ifndef __SNTP_TIME_H__
#define __SNTP_TIME_H__

#include <time.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化SNTP时间同步服务
 * @note 需要在WiFi连接成功后调用
 */
void sntp_time_init(void);

/**
 * @brief 停止SNTP服务
 */
void sntp_time_stop(void);

/**
 * @brief 检查时间是否已同步
 * @return true 已同步，false 未同步
 */
bool sntp_time_is_synced(void);

/**
 * @brief 获取当前时间结构体
 * @param timeinfo 输出时间信息
 * @return ESP_OK 成功，ESP_FAIL 失败
 */
esp_err_t sntp_time_get(struct tm *timeinfo);

/**
 * @brief 获取格式化的时间字符串
 * @param buf 输出缓冲区
 * @param buf_len 缓冲区长度
 * @param format 时间格式（strftime格式），NULL则使用默认格式"%Y-%m-%d %H:%M:%S"
 * @return ESP_OK 成功，ESP_FAIL 失败
 */
esp_err_t sntp_time_get_str(char *buf, size_t buf_len, const char *format);

/**
 * @brief 获取日期字符串 (YYYY-MM-DD)
 */
esp_err_t sntp_time_get_date(char *buf, size_t buf_len);

/**
 * @brief 获取时间字符串 (HH:MM:SS)
 */
esp_err_t sntp_time_get_time(char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif // __SNTP_TIME_H__
