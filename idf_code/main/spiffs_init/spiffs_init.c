#include "spiffs_init.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "SPIFFS";
static bool s_spiffs_mounted = false;

// 路径缓冲区大小：挂载点(8) + 斜杠(1) + 文件名(255) + 终止符(1) = 265，取320
#define PATH_BUF_SIZE 320

esp_err_t spiffs_init(void)
{
    if (s_spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_MOUNT_POINT,
        .partition_label = NULL,  // 使用默认分区
        .max_files = SPIFFS_MAX_FILES,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted. Total: %d KB, Used: %d KB", total/1024, used/1024);
    }

    s_spiffs_mounted = true;
    return ESP_OK;
}

void spiffs_deinit(void)
{
    if (s_spiffs_mounted) {
        esp_vfs_spiffs_unregister(NULL);
        s_spiffs_mounted = false;
        ESP_LOGI(TAG, "SPIFFS unmounted");
    }
}

bool spiffs_is_mounted(void)
{
    return s_spiffs_mounted;
}

esp_err_t spiffs_get_info(size_t *total, size_t *used)
{
    if (!s_spiffs_mounted) {
        return ESP_FAIL;
    }
    return esp_spiffs_info(NULL, total, used);
}

bool spiffs_file_exists(const char *path)
{
    char full_path[PATH_BUF_SIZE];
    snprintf(full_path, sizeof(full_path), "%s%s", SPIFFS_MOUNT_POINT, path);
    
    struct stat st;
    return (stat(full_path, &st) == 0);
}

esp_err_t spiffs_delete_file(const char *path)
{
    char full_path[PATH_BUF_SIZE];
    snprintf(full_path, sizeof(full_path), "%s%s", SPIFFS_MOUNT_POINT, path);
    
    if (unlink(full_path) == 0) {
        ESP_LOGI(TAG, "Deleted file: %s", path);
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Failed to delete file: %s", path);
    return ESP_FAIL;
}

void spiffs_list_files(void)
{
    DIR *dir = opendir(SPIFFS_MOUNT_POINT);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory");
        return;
    }

    ESP_LOGI(TAG, "Files in SPIFFS:");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_BUF_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", SPIFFS_MOUNT_POINT, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            ESP_LOGI(TAG, "  %s (%ld bytes)", entry->d_name, st.st_size);
        }
    }
    closedir(dir);
}
