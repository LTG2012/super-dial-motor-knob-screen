#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "lvgl.h"
#include "ui.h"

#define SIM_HOR_RES 240
#define SIM_VER_RES 240

static SDL_Window *s_window = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture *s_texture = NULL;

static void sdl_cleanup(void)
{
    if(s_texture) {
        SDL_DestroyTexture(s_texture);
        s_texture = NULL;
    }
    if(s_renderer) {
        SDL_DestroyRenderer(s_renderer);
        s_renderer = NULL;
    }
    if(s_window) {
        SDL_DestroyWindow(s_window);
        s_window = NULL;
    }
    SDL_Quit();
}

static void sdl_init(void)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }

    s_window = SDL_CreateWindow("SuperDial UI Simulator",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                SIM_HOR_RES, SIM_VER_RES, 0);
    if(!s_window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(1);
    }

    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED);
    if(!s_renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }

    s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  SIM_HOR_RES, SIM_VER_RES);
    if(!s_texture) {
        printf("SDL_CreateTexture failed: %s\n", SDL_GetError());
        exit(1);
    }
}

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    SDL_Rect rect = { .x = area->x1, .y = area->y1, .w = w, .h = h };

#if LV_COLOR_16_SWAP
    static uint16_t *swap_buf = NULL;
    static size_t swap_buf_len = 0;
    size_t needed = (size_t)(w * h);
    if(needed > swap_buf_len) {
        uint16_t *new_buf = (uint16_t *)realloc(swap_buf, needed * sizeof(uint16_t));
        if(!new_buf) {
            lv_disp_flush_ready(disp_drv);
            return;
        }
        swap_buf = new_buf;
        swap_buf_len = needed;
    }
    for(size_t i = 0; i < needed; ++i) {
        uint16_t px = color_p[i].full;
        swap_buf[i] = (uint16_t)((px >> 8) | (px << 8));
    }
    SDL_UpdateTexture(s_texture, &rect, swap_buf, w * (int)sizeof(uint16_t));
#else
    SDL_UpdateTexture(s_texture, &rect, color_p, w * (int)sizeof(lv_color_t));
#endif

    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);

    lv_disp_flush_ready(disp_drv);
}

static void lv_port_init(void)
{
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[SIM_HOR_RES * 40];

    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SIM_HOR_RES * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SIM_HOR_RES;
    disp_drv.ver_res = SIM_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    sdl_init();
    lv_init();
    lv_port_init();
    ui_init();

    uint32_t last_tick = SDL_GetTicks();
    bool running = true;

    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = false;
            }
        }

        uint32_t now = SDL_GetTicks();
        lv_tick_inc(now - last_tick);
        last_tick = now;

        lv_timer_handler();
        SDL_Delay(5);
    }

    sdl_cleanup();
    return 0;
}
