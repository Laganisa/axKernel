#pragma region include_Header

// 타입 헤더
#include "types.h"

// 분리 파일
#include "asm.h"  // 어셈블리 함수가 있는 헤더
#include "defs.h" // 정의 헤더
#include "sect.h" // 메모리 매핑 헤더

#include "io.h"   // 입출력 헤더
#include "irq.h"  // 인터럽트 헤더 추가
#include "sync.h" // Exception handlers

#include "mm.h" // 메모리 관리자가 있는 헤더
#include "pm.h" // 프로세스 관리자 헤더
#include "fm.h" // 파일 관리자 헤더

extern void _proc(pcb_t *);
extern void vector_table(void);
// extern void irq_handler_main(void);

// 쉘 코드
extern uint8_t _shell_binary_start[];
extern uint8_t _shell_binary_size[];

volatile uint8_t resched_flag = 0;
int current_task_id = 0; // 반드시 함수 밖(Global)에 있어야 함

#pragma endregion

#include "meta.h"
#include "debug.h"
#include "in_proc.h"

// 커널 함수
void main(void)
{
#pragma region reset
    // 하드웨어 초기화
    uart_init();
    // 관리자 초기화
    // MM 메타데이터는 MM_ADDR_START에 두고, 실제 프로세스 메모리는 사용자 영역에서 시작한다.
    mm_init(&mm_stack, USER_PROC_START);
    // pm_init(&pm_object, PM_ADDR_START);
    fm_init((uint64_t *)USER_FILE_START);
    // 인터럽트/타이머 초기화

#pragma endregion

    FMv2_record *reco = (FMv2_record *)FM_ADDR_START;

    puts("Booting AxKernel\n");

    // 파일로 실행해보기
    pcb_t *proc1 = proc_turn(reco, "TA.BIN", &task_inf_A, 0);

    // pcb_t *proc2 = proc_turn(reco, "TB.BIN", &task_inf_B, 0);
    // pm_awake(&pm_object, 0, proc2);

    // pcb_t *shell_p = proc_turn(reco, "SHEL.BIN", _shell_binary_start, 1);
    // pm_awake(&pm_object, 0, shell_p);

    proc_dump("proc1", proc1);
    // proc_dump("proc2", proc2);

#pragma region proc_change

    init_irq();

    current_proc = proc1;
    _proc(proc1);

#pragma endregion
}
