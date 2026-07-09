#include "io.h"
#include "irq.h"
#include "sync.h"
#include "syscall.h"

#include "debug.h"

/*
    63개의 sync 핸들러
*/

// ! sp 기준 스위칭을 수정하기
uint64_t handle_data_abort(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("handle_data_abort");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    pcb_t *current = get_current_proc_addr();
    pcb_t *next;

    pm_awake(&pm_object, 1, current);
    next = pm_run(&pm_object);
    current_proc = next;

    if (next != 0)
    {
        new_context(next->sp);
    }

    full_stop();
    return 0;
}

// 명령어 접근 오류
uint64_t handle_inst_abort(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("handle_inst_abort");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    full_stop();
    return 0;
}
