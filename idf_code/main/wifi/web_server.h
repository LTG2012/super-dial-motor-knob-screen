#pragma once

#include "esp_http_server.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 启动Web服务器
httpd_handle_t start_webserver(void);

// 停止Web服务器
void stop_webserver(httpd_handle_t server);

#ifdef __cplusplus
}
#endif
