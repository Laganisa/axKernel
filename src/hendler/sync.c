#include "io.h"
#include "irq.h"
#include "sync.h"
#include "syscall.h"

#include "debug.h"

/*
    헨들러 예외 처리를 C로 처리하는 파일
*/

void sync_handler_main(uint64_t sys_call, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t ec)
{
    // 시스템 콜 리턴값을 받을 변수
    uint64_t val;

    // ! 나중에 함수 포인터로 바꿀 예정
    switch (ec)
    {
    case 0x20:
        handle_inst_abort();
        break;
    case 0x15:
        val = handle_syscall(sys_call, arg1, arg2, arg3);
        break;
    case 0x24:
        handle_data_abort();
        break;
    default:
        // sync 에러
        check_el0_irq();
        break;
    }

    return val;
}

// el0에서 데이터 어보트가 나와서 호출됨
// far, elr, esr을 출력하고 종료
void handle_data_abort(void)
{

    reg_far_el1();
    reg_elr_el1();

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
}

// 명령어 접근 오류
void handle_inst_abort(void)
{
    reg_far_el1();
    reg_elr_el1();

    full_stop();
}