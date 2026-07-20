#include "global/_io.h"
#include "handler/_irq.h"
#include "handler/_sync.h"
#include "handler/_syscall.h"
#include "global/_debug.h"

/*
    헨들러 예외 처리를 C로 처리하는 파일
*/

uint64_t (*ec_table[64])(uint64_t, uint64_t, uint64_t, uint64_t) = {
    [0x0] = &handle_unknown,
    [0x15] = &handle_svc_a64,
    [0x20] = &handle_inst_abort,
    [0x24] = &handle_data_abort,
};

uint64_t sync_handler_main(uint64_t sys_call, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t ec)
{
    // sync 리턴값을 받을 변수
    // 시스템 콜 리턴값을 받을 변수
    uint64_t val;

    // ec 값에 맞는 함수 실행
    val = ec_table[ec](sys_call, arg1, arg2, arg3);

    return val;
}

// 알 수 없는 ec 명령
uint64_t handle_unknown(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("handle_unknown");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}