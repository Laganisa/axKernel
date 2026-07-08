#include "../include/pm.h"

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

// 예외 핸들러 선언
void handle_data_abort(void);
void handle_inst_abort(void);

void sync_handler_main(
    uint64_t sys_call,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t ec);

extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);

// void new_context(uint64_t sp);

#endif
