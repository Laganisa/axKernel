// void main에서 void kernel로 전환해야 할듯
// main을 발사대로만

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

volatile uint8_t resched_flag = 0;
int current_task_id = 0; // 반드시 함수 밖(Global)에 있어야 함

void task_B1()
{
    // puts("[TASK CHECK]");
    // put_hex(current_proc->id);

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

    while (1)
    {
        int8_t cmd[64];
        puts("root@localhost : ");
        remo_get(cmd, 64);

        if (strcmp(cmd, "end") == 0)
        {
            // 시스템 종료
            break;
        }
        else if (strcmp(cmd, "help") == 0)
        {
            // 어떤 쉘 명령이 있는지
            knowcmd();
        }
        else
        {
            // 쉘 명령 실행
            shell_run(cmd);
        }
    }

    puts("Goodbye, see you next time."); // 종료 메시지
}

/*
    pid 2 : 네트워크 프로세스
    첫 번째 데몬

*/
void NET(void)
{
    // 네트워크 관리자
}

/*
    pid 3 : 디바이스 관리자
    2번째 데몬
*/
void DEV(void)
{
    // 디바이스 관리자
}

void task_A()
{
    // puts("[TASK CHECK]");
    // put_hex(current_proc->id);

    asm volatile("msr daifclr, #2");

    while (1)
    {
        puts("A");
        for (volatile int i = 0; i < 100000; i++)
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
        "mov x8, #93\n"
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
    // 하드웨어 초기화
    uart_init();
    // 관리자 초기화
    // MM 메타데이터는 MM_ADDR_START에 두고, 실제 프로세스 메모리는 사용자 영역에서 시작한다.
    mm_init(&mm_stack, USER_PROC_START);
    // pm_init(&pm_object, PM_ADDR_START);
    fm_init(USER_FILE_START);
    // 인터럽트/타이머 초기화

    FMv2_record *reco = (FMv2_record *)FM_ADDR_START;

    int8_t task_a_path[27] = "TA.BIN";
    int8_t task_b_path[27] = "TB.BIN";

    fm_exec_hdr_t task_a_exec;

    task_a_exec.magic = FM_EXEC_MAGIC;

    task_a_exec.mode = FM_EXEC_MODE_DIRECT;

    task_a_exec.entry = (uint64_t)&task_A;

    task_a_exec.image_size = 0;

    fm_exec_hdr_t task_b_exec;

    task_b_exec.magic = FM_EXEC_MAGIC;

    task_b_exec.mode = FM_EXEC_MODE_DIRECT;

    task_b_exec.entry = (uint64_t)&task_B;

    task_b_exec.image_size = 0;

    // puts("--- FM Memory Direct Check ---\n");

    // 파일 생성 (루트에 abcd.txt 생성)
    // 인자: (관리객체, 이름, 크기, 디렉토리여부, 경로)
    fm_create(reco, task_a_path, 1024, 0);
    fm_create(reco, task_b_path, 1024, 0);
    fm_write(reco, task_a_path, &task_a_exec, sizeof(task_a_exec), 0);
    fm_write(reco, task_b_path, &task_b_exec, sizeof(task_b_exec), 0);
    // 관리자 정보 직접 확인
    // puts("Current last_addr: 0x");
    // put_hex(reco->last_addr); // 할당 후 0 -> 1이 되었는지 확인
    // puts("\n");

    // 매핑 테이블 직접 확인 (루트 슬롯 0번)
    puts("Mapping[0][16][16]: 0x");
    put_hex(reco->mapping[0][16][16]); // token 값이 들어갔는지 확인
    puts("\n");

    // 5. 실제 데이터 영역(FCB) 직접 확인
    // 설계상 FMv2_mem[0][16][16] 위치
    fcb_t *check_fcb = &(reco->FMv2_mem[0][16][16]);

    puts("Allocated FID: 0x");
    put_hex(check_fcb->fid);
    puts("\n");

    puts("Stored Alias: ");
    for (int i = 0; i < 8; i++)
    {
        if (check_fcb->alias[i] != 0)
            putchar(check_fcb->alias[i]);
    }

    puts("\n---------------------------\n");
    put_hex(sizeof(PMv1_object));
    puts("myOS kernel\n");                 // 부팅 메시지
    puts("'help' : list commands\n");      // 사용 가능한 명령어 확인
    puts("'end'  : exit\n");               // 시스템 나가기
    puts("Welcome! Have a great time.\n"); // 환영 메시지

    pcb_t *proc0 = creat_proc(&pm_object, &INIT, 0);
    pcb_t *proc1 = fm_exec_file(reco, &pm_object, task_a_path, 0);
    pcb_t *proc2 = fm_exec_file(reco, &pm_object, task_b_path, 0);

    if (proc0 == 0 || proc1 == 0 || proc2 == 0)
    {
        puts("proc create failed\n");
        puts("proc0: ");
        put_hex((uint64_t)proc0);
        puts("proc1: ");
        put_hex((uint64_t)proc1);
        puts("proc2: ");
        put_hex((uint64_t)proc2);
        while (1)
            ;
    }

    current_proc = proc1;

    put_hex(proc0->id);
    put_hex(proc1->id);
    put_hex(proc2->id);

    puts("a");
    pm_awake(&pm_object, 0, proc2);
    puts("a");
    pm_awake(&pm_object, 0, proc0);
    puts("a");
    // ? pm_awake(&pm_object, 0, proc1);

    init_irq();

    _proc((uint64_t *)proc1->sp);
}
