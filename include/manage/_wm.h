#ifndef KERNEL_WM_H
#define KERNEL_WM_H

#include "_types.h"

typedef struct wcb_t
{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;

    uint8_t is_on;

    void (*draw)(uint32_t, uint32_t);
} wcb_t;

typedef struct WMv1_slot
{
    wcb_t wm_arr[9];
    uint8_t cnt;
} WMv1_slot;

#define wm_object ((WMv1_slot *)GM_ADDR_START)

uint16_t wputs(const char *s);
uint16_t wputs_color(const char *s, uint32_t color);
int32_t wget(char *s, int32_t max_len);
void wconsole_erase_char(void);

extern uint32_t wm_console_line;
extern uint32_t wm_console_xpixel;

#endif
