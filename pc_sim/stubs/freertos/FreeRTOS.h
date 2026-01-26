#pragma once

#include <stdint.h>

typedef uint32_t TickType_t;
typedef void * QueueHandle_t;

#define pdPASS 1
#define portTICK_PERIOD_MS 1

static inline int xQueueOverwrite(QueueHandle_t queue, const void *item)
{
    (void)queue;
    (void)item;
    return pdPASS;
}
