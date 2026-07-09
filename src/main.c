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

#include "meta.h"
#include "debug.h"
#include "in_proc.h"

extern void _proc(pcb_t *);
extern void vector_table(void);

// 쉘 코드
extern uint8_t _task_shell_start[];
extern uint8_t _task_shell_size[];

#pragma endregion

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

    // 초기화 부분으로 옮김
    init_irq();

#pragma endregion

    FMv2_record *reco = (FMv2_record *)FM_ADDR_START;

    puts("Booting AxKernel\n");

    /*
        파일로 만든뒤 대기 큐에 넣기
    */

    // pcb_t *proc1 = proc_turn(reco, "TA.BIN", &task_inf_A, 0);

    // pcb_t *proc2 = proc_turn(reco, "TB.BIN", &task_inf_B, 0);
    // pm_awake(&pm_object, 0, proc2);

    pcb_t *shell_proc = proc_turn(reco, "SHEL.BIN", _task_shell_start, 1);
    pm_awake(&pm_object, 0, shell_proc);

    fcb_t *a_file = fm_create(reco, "a", 1, 0);

    // proc_dump("proc1", proc1);
    // proc_dump("proc2", proc2);
    proc_dump("shell proc", shell_proc);
    file_dump("a file", a_file);

    /*
        프로세스 전환
    */

    current_proc = shell_proc;
    _proc(shell_proc);
}