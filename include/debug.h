#include "types.h"

#ifndef __DEBUG_H__
#define __DEBUG_H__

void dump(const char *name, uint64_t val);
void full_stop(void);
void enter(const char *name);
void log(const char *name);

void reg_x8(void);
void reg_elr_el1(void);
void reg_esr_el1(void);
void reg_far_el1(void);
void reg_vbar(void);

#endif