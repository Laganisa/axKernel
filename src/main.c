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
#include "global/_dtb.h"

#include "manage/_mm.h" // 메모리 관리자 헤더
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

// 컴파일러 코드
extern uint8_t _task_compiler_start[];
extern uint64_t _task_compiler_size[];

// 브릿지 코드
extern uint8_t _task_bridge_start[];
extern uint64_t _task_bridge_size[];

extern dcb_t nic_device;

#pragma endregion

#define B_MASTER_FLAG 1

// 커널 함수
void master(uint64_t dtb_addr)
{
    dump("Passed_DTB_addr", dtb_addr);

    // dtb 파싱 후 연동
    parse_dtb(dtb_addr);

#if defined(NET)
    net_RX_main();
#elif B_MASTER_FLAG == 0
    net_TX_main();
#elif B_MASTER_FLAG == 1
    kernel_main();
#elif B_MASTER_FLAG == 2
    devo_main();
#else
    kernel_main();
#endif
}

// 임시 함수로 빼기전 개발용 매인
void devo_main(void)
{
    puts("devo main\n");
    gic_init();

    blk_init();
}

#define B_MAIN_FLAG 2

void kernel_main(void)
{

    // 하드웨어 초기화
    uart_init();
    // 인터럽트 초기화
    irq_init();
    // 동적할당 초기화
    heap_init();

    // 자료구조 초기화

    // 관리자 초기화
    mm_init(&mm_stack, USER_PROC_START);
    fm_init((uint64_t *)USER_FILE_START);
    pm_init();

    nm_init();
    gpu_init();
    /*
        그래픽을 사용하여 부팅 로그를 만들기
    */

    ltrs(0, 0, "Booting AxKernel!");

    puts("Booting AxKernel!\n");

    /*
        파일 생성 후 프로세스로 만든뒤 대기 큐에 넣기
        나중에 각각 ROOT 프로세스, INIT 프로세스가 될 예정
    */

    pcb_t *proc1 = proc_turn(fm_record, "devo.BIN", devo_main, 0);

    proc_dump("proc1", proc1);

    /*
        프로세스 전환
    */

    current_proc = proc1;
    _proc(proc1);

#ifdef defined B_MAIN_FLAG

#elif B_MAIN_FLAG == 1
    // 프로세스 전환 테스트 로직

    pcb_t *proc1 = proc_turn(fm_record, "INFA.BIN", task_inf_A, 0);

    pcb_t *proc2 = proc_turn(fm_record, "INFB.BIN", task_inf_B, 0);
    pm_awake(&pm_object, 0, proc2);

    proc_dump("proc1", proc1);
    proc_dump("proc2", proc2);

    /*
        프로세스 전환
    */

    dump("1", pm_object.lowbuf[0]);

    current_proc = proc1;
    _proc(proc1);

#elif B_MAIN_FLAG == 2

    // 쉘 테스트 로직
    pcb_t *shell_proc = proc_turn(fm_record, "shel.bin", _task_shell_start, 1);
    pm_awake(&pm_object, 0, shell_proc);

    proc_dump("shell proc", shell_proc);

    current_proc = shell_proc;
    _proc(shell_proc);

#elif B_MAIN_FLAG == 3
    // 브릿지 테스트 로직

    pcb_t *brdge_proc = proc_turn(fm_record, "brdge.bin", _task_bridge_start, 1);
    pm_awake(&pm_object, 0, brdge_proc);

    proc_dump("brdge proc", brdge_proc);

    current_proc = brdge_proc;
    _proc(brdge_proc);

#elif B_MAIN_FLAG == 4
    // 컴파일러 테스트 로직

    pcb_t *compil_proc = proc_turn(
        fm_record,
        "compil.bin",
        _task_compiler_start,
        1);

    pm_awake(&pm_object, 0, compil_proc);

    proc_dump("compil proc", compil_proc);

    current_proc = compil_proc;
    _proc(compil_proc);
#elif B_MAIN_FLAG == 5

    // ipc 테스트 로직

    /*
        쉘이랑 브릿지 2개를 띄워서 테스트
    */
    pcb_t *bridge_proc = proc_turn(fm_record, "bridge.bin", _task_bridge_start, 1);
    pm_awake(&pm_object, 0, bridge_proc);

    pcb_t *shell_proc = proc_turn(fm_record, "shel.bin", _task_shell_start, 1);
    pm_awake(&pm_object, 0, shell_proc);

    proc_dump("shell proc", bridge_proc);
    proc_dump("shell proc", shell_proc);

    current_proc = shell_proc;
    _proc(shell_proc);

#endif
}