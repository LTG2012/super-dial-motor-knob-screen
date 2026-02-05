#ifndef __SPIFFS_INIT_H__
#define __SPIFFS_INIT_H__

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPIFFS_MOUNT_POINT "/spiffs"
#define SPIFFS_MAX_FILES 10

/**
 * @brief 初始化并挂载SPIFFS文件系统
 * @return ESP_OK 成功
 */
esp_err_t spiffs_init(void);

/**
 * @brief 卸载SPIFFS文件系统
 */
void spiffs_deinit(void);

/**
 * @brief 检查SPIFFS是否已挂载
 */
bool spiffs_is_mounted(void);

/**
 * @brief 获取SPIFFS存储信息
 * @param total 总容量（字节）
 * @param used 已使用（字节）
 * @return ESP_OK 成功
 */
esp_err_t spiffs_get_info(size_t *total, size_t *used);

/**
 * @brief 检查文件是否存在
 * @param path 文件路径（相对于挂载点）
 */
bool spiffs_file_exists(const char *path);

/**
 * @brief 删除文件
 * @param path 文件路径（相对于挂载点）
 */
esp_err_t spiffs_delete_file(const char *path);

/**
 * @brief 列出目录内容（用于调试）
 */
void spiffs_list_files(void);

#ifdef __cplusplus
}
#endif

#endif // __SPIFFS_INIT_H__
