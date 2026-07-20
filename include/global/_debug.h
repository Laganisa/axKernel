#include "_types.h"
#include "manage/_pm.h"
#include "manage/_fm.h"

#ifndef __DEBUG_H__
#define __DEBUG_H__

void dump(const char *name, uint64_t val);
void full_stop(void);
void enter(const char *name);
void log(const char *name);
void flow(const char type);
void exit(const char *name);

void reg_x8(void);
void reg_elr_el1(void);
void reg_esr_el1(void);
void reg_far_el1(void);
void reg_vbar(void);

void check_el1_sync(void);
void check_el0_sync(void);
void check_el1_irq(void);
void check_el0_irq(void);
void check_inf_loop(void);
void flow_asm(void);
void check_loop(void);
void proc_dump(const char *name, pcb_t *proc);
void file_dump(const char *name, fcb_t *file);

#endif