#include "_defs.h"
#include "_sect.h"
#include "_types.h"

#ifndef __IO_H__
#define __IO_H__

void uart_init(void);
void putchar(int8_t c);
void puts(const int8_t *s);
int8_t getchar(void);
void gets(int8_t *s, int32_t max_len);

void put_hex(uint64_t d);
void put_uint(uint64_t n);

int32_t strcmp(const int8_t *s1, const int8_t *s2);

#endif
