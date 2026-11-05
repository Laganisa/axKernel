#ifndef KERNEL_GM_H
#define KERNEL_GM_H

#include "_defs.h"
#include "_types.h"
#include "tools/_font.h"
#include "_sect.h"

enum GM_KEY_FLAG
{
    FLAG_NON = 0,
    CRTL_A,
};

typedef struct wcb_t
{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;

    uint8_t is_on;

    void (*draw)(uint32_t, uint32_t);
} wcb_t;

typedef struct WMv1_window
{
    wcb_t wm_arr[9];
    uint8_t cnt;
} WMv1_window;

void gm_pixel(uint32_t x, uint32_t y, uint32_t color);

void gm_full(uint32_t color);

void gm_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color);

/* Copies the rectangle between inclusive corner coordinates. */
void gm_copy(
    uint32_t x1,
    uint32_t y1,
    uint32_t x2,
    uint32_t y2);

void gm_paste(uint32_t x, uint32_t y);

void gm_line(
    uint32_t x1,
    uint32_t y1,
    uint32_t x2,
    uint32_t y2,
    uint32_t color);

#define wm_object ((WMv1_window *)GM_ADDR_START)

#endif