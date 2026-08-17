#ifndef KERNEL_GM_H
#define KERNEL_GM_H

#include "_defs.h"
#include "_types.h"

void draw_pixel(uint32_t x, uint32_t y, uint32_t color);

void gpu_fill_screen(uint32_t color);

void gpu_part_screen(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color);

#endif