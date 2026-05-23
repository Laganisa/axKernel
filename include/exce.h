#include "../include/pm.h"

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

// 예외 핸들러 선언
void curr_el_sp0_sync(void);
void curr_el_sp0_irq(void);
void curr_el_sp0_fiq(void);
void curr_el_sp0_serror(void);

void curr_el_spx_sync(void);
void curr_el_spx_irq(void);
void curr_el_spx_fiq(void);
void curr_el_spx_serror(void);

void lower_el_aarch64_sync(void);
void lower_el_aarch64_fiq(void);
void lower_el_aarch64_serror(void);

void lower_el_aarch32_sync(void);
void lower_el_aarch32_irq(void);
void lower_el_aarch32_fiq(void);
void lower_el_aarch32_serror(void);

extern pcb_t *current_proc;
extern pcb_t **get_current_proc_addr(void);
void new_context(uint64_t sp);

#endif
