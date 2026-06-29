#include "../include/pm.h"

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

// 예외 핸들러 선언

// Data abort handler helper called from exception asm
void handle_data_abort(uint64_t far, uint64_t elr, uint64_t esr);

extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);
void new_context(uint64_t sp);

#endif
