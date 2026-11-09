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
    uint8_t pcb_id;

    void (*draw)(uint32_t, uint32_t);
} wcb_t;

typedef struct gui_cmd_t
{
    uint8_t type;
    uint8_t pcb_id;

    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;

    uint32_t color;

    const char *str;
    uint16_t len;

} gui_cmd_t;

#define WM_CMD_PUTS 1
#define WM_CMD_QUEUE_SIZE 16

typedef struct WMv1_slot
{
    wcb_t wm_arr[3];
    uint8_t now_pcb_id;
    uint8_t cnt;

    gui_cmd_t cmd_arr[WM_CMD_QUEUE_SIZE];
    uint8_t cmd_cnt;
    char str_buf[512];

} WMv1_slot;

#define wm_slot ((WMv1_slot *)GM_ADDR_START)

uint16_t wputs(const char *s);
uint16_t wputs_color(const char *s, uint32_t color);
int32_t wget(char *s, int32_t max_len);
void wconsole_erase_char(void);
void wm_switch(void);

extern uint32_t wm_console_line;
extern uint32_t wm_console_xpixel;

#endif
