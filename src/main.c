#pragma region include_Header

// 타입 헤더
#include "../include/types.h"

// 분리 파일
#include "../include/asm.h"  // 어셈블리 함수가 있는 헤더
#include "../include/defs.h" // 정의 헤더
#include "../include/sect.h" // 메모리 매핑 헤더

#include "../include/io.h"   // 입출력 헤더
#include "../include/irq.h"  // 인터럽트 헤더 추가
#include "../include/exce.h" // Exception handlers

#include "../include/mm.h" // 메모리 관리자가 있는 헤더
#include "../include/pm.h" // 프로세스 관리자 헤더
#include "../include/fm.h" // 파일 관리자 헤더

extern void _proc(uint64_t *reg_val);
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

void task_B1()
{

    asm volatile("msr daifclr, #2");

    while (1)
    {
        puts("B");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

/*
    pid 0 : 루트 프로세스
    pid 1 의 부모
*/
void ROOT(void)
{
    // 루트 프로세스
}

/*
    pid 1 :init 프로세스
    데몬들의 부모이자 모든 프로세스의 부모
*/
void INIT(void)
{
    asm volatile("msr daifclr, #2");
}

void task_A()
{
    asm volatile("msr daifclr, #2");

    while (1)
    {
        puts("A");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_B()
{
    // puts("[TASK CHECK]");
    // put_hex(current_proc->id);

    asm volatile("msr daifclr, #2");
    int i = 10;
    while (i > 0)
    {
        puts("B");
        i--;
    }

    asm volatile(
        "mov x8, #1\n"
        "svc #0\n" ::: "x8" // x8 레지스터를 사용한다고 컴파일러에게 알림
    );

    while (1)
    {
        puts("Error: Task B should be dead!\n");
    }
}

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

// 이거 쉘으로 넘기기
#pragma region booting msg

    puts("axOS kernel\n");                 // 부팅 메시지
    puts("'help' : list commands\n");      // 사용 가능한 명령어 확인
    puts("'end'  : exit\n");               // 시스템 나가기
    puts("Welcome! Have a great time.\n"); // 환영 메시지

#pragma endregion

    // 파일로 실행해보기

    pcb_t *proc1 = proc_turn(reco, "TA.BIN", &task_A, 0);

    pcb_t *proc2 = proc_turn(reco, "TB.BIN", &task_B, 0);

    pcb_t *proc3 = proc_turn(reco, "TC.BIN", _shell_binary_start, 1);

    // full_stop();

#pragma region proc_change

    // 깨우기
    pm_awake(&pm_object, 0, proc2);
    pm_awake(&pm_object, 0, proc3);

    init_irq();

    current_proc = proc1;
    _proc((uint64_t *)proc1->sp);

#pragma endregion
}
