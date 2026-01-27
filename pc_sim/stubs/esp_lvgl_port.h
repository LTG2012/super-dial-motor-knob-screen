#pragma once

static inline void lvgl_port_lock(int timeout_ms)
{
    (void)timeout_ms;
}

static inline void lvgl_port_unlock(void)
{
}
