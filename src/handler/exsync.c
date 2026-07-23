#include "global/_io.h"
#include "handler/_irq.h"
#include "handler/_sync.h"
#include "handler/_syscall.h"
#include "global/_debug.h"

extern void _proc(pcb_t *);

/*
    63개의 sync 핸들러
*/

uint64_t handle_data_abort(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("handle_data_abort");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    pcb_t *current = get_current_proc_addr();
    pcb_t *next;

    // 현재 프로세스를 비활성 상태로 변경 후 다음 프로세스 꺼내기
    pm_awake(&pm_object, 1, current);
    next = pm_run(&pm_object);
    current_proc = next;

    if (next != 0)
    {
        _proc(next);
    }

    // 절대 오면 안되는 구역
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
