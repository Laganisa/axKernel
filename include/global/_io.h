#ifndef __KERNEL_IO_H__
#define __KERNEL_IO_H__

#include "_defs.h"
#include "_sect.h"
#include "_types.h"

enum io_key_code
{
    IO_KEY_F1 = -1,
    IO_KEY_F2 = -2,
    IO_KEY_F3 = -3,
    IO_KEY_F4 = -4,
    IO_KEY_F5 = -5,
    IO_KEY_F6 = -6,
    IO_KEY_F7 = -7,
    IO_KEY_F8 = -8,
    IO_KEY_F9 = -9,
    IO_KEY_F10 = -10,
    IO_KEY_F11 = -11,
    IO_KEY_F12 = -12,
    IO_KEY_ESC = -13,
    IO_KEY_CTRL = -14,
    IO_KEY_UP = -15,
    IO_KEY_DOWN = -16,
    IO_KEY_LEFT = -17,
    IO_KEY_RIGHT = -18
};

void uart_init(void);
void putchar(int8_t c);
void puts(const int8_t *s);
/* Keyboard special keys return IO_KEY_*; ordinary input retains ASCII values. */
int8_t getchar(void);
void gets(int8_t *s, int32_t max_len);
uint16_t io_puts_color(const char *s, uint32_t color);

extern uint32_t io_console_line;
extern uint32_t io_console_xpixel;

void put_hex(uint64_t d);
void put_uint(uint64_t n);
void put_hex2(uint64_t d);

int32_t strcmp(const int8_t *s1, const int8_t *s2);
void *memset(void *ptr, int value, size_t num);
char *strstr(const char *s, const char *sub);
uint64_t str_to_uint(const char *str);

#endif
