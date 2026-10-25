#ifndef __KERNEL_EXCEPTION_H__
#define __KERNEL_EXCEPTION_H__

#include "manage/_pm.h"

extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);

uint64_t sync_handler_main(
    uint64_t sys_call,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t ec);

// 예외 핸들러 선언
uint64_t svc_a64_handle(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

#endif
