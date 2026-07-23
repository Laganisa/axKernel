#pragma region include_GOD_Header

// 기초 헤더
#include "_types.h"
#include "_defs.h" // 정의 헤더
#include "_sect.h" // 메모리 매핑 헤더
#include "_macro.h"

// 분리 파일
#include "global/_io.h" // 입출력 헤더
#include "global/_meta.h"
#include "global/_debug.h"
#include "global/_in_proc.h"
#include "global/_alloc.h"

#include "manage/_mm.h" // 메모리 관리자가 있는 헤더
#include "manage/_pm.h" // 프로세스 관리자 헤더
#include "manage/_fm.h" // 파일 관리자 헤더
#include "manage/_nm.h" // 네트워크 관리자 헤더
#include "manage/_gm.h" // 그래픽 관리자 헤더

#include "handler/_irq.h"     // 인터럽트 헤더 추가
#include "handler/_sync.h"    // 예외 핸들러 sync
#include "handler/_syscall.h" // 시스템 콜 헨들러

#include "tools/_asm.h" // 어셈블리 함수가 있는 헤더

extern void _proc(pcb_t *);
extern void vector_table(void);

// 쉘 코드
extern uint8_t _task_shell_start[];
extern uint8_t _task_shell_size[];

extern dcb_t nic_device;

#pragma endregion

// 커널 함수
void master(void)
{
    // 디버깅 하기
    debug_main();
}

void kernel_main(void)
{ // 하드웨어 초기화
    uart_init();
    // 인터럽트 초기화
    init_irq();
    // 동적할당 초기화
    heap_init();

    // 관리자 초기화
    mm_init(&mm_stack, USER_PROC_START);
    // pm_init(&pm_object, PM_ADDR_START);
    fm_init((uint64_t *)USER_FILE_START);

    puts("Booting AxKernel!\n");

    /*
        파일 생성 후 프로세스로 만든뒤 대기 큐에 넣기
    */

    // pcb_t *proc1 = proc_turn(reco, "TA.BIN", &task_inf_A, 0);

    // pcb_t *proc2 = proc_turn(reco, "TB.BIN", &task_inf_B, 0);
    // pm_awake(&pm_object, 0, proc2);

    pcb_t *shell_proc = proc_turn(fm_record, "SHEL.BIN", _task_shell_start, 1);
    pm_awake(&pm_object, 0, shell_proc);

    // proc_dump("proc1", proc1);
    // proc_dump("proc2", proc2);
    proc_dump("shell proc", shell_proc);

    /*
        프로세스 전환
    */

    current_proc = shell_proc;
    _proc(shell_proc);
}

void devo_main(void)
{
}