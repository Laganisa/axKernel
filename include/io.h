#include "../include/defs.h"
#include "../include/sect.h"

#ifndef __IO_H__
#define __IO_H__

void uart_init(void);
void putchar(int8_t c);
void puts(const int8_t *s);
int8_t getchar(void);
void gets(int8_t *s, int32_t max_len);
void remo_get(int8_t *s, int32_t max_len);

void put_hex(uint64_t d);
void put_uint(uint64_t n);
void clear(void);
void shell_run(int8_t *cmd);
void knowcmd(void);
int32_t strcmp(const int8_t *s1, const int8_t *s2);


// PCB-related
extern void *current_pcb_addr;

#endif
