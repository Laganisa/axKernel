#ifndef KERNEL_GM_H
#define KERNEL_GM_H

#include "_defs.h"
#include "_types.h"
#include "tools/_font.h"

void draw_pixel(uint32_t x, uint32_t y, uint32_t color);

void gm_full(uint32_t color);

void gm_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color);

#endif