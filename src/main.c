#pragma region include_Header

// 타입 헤더
#include "_types.h"

// 분리 파일
#include "_asm.h"  // 어셈블리 함수가 있는 헤더
#include "_defs.h" // 정의 헤더
#include "_sect.h" // 메모리 매핑 헤더

#include "_io.h"   // 입출력 헤더
#include "_irq.h"  // 인터럽트 헤더 추가
#include "_sync.h" // Exception handlers

#include "_mm.h" // 메모리 관리자가 있는 헤더
#include "_pm.h" // 프로세스 관리자 헤더
#include "_fm.h" // 파일 관리자 헤더

#include "_meta.h"
#include "_debug.h"
#include "_in_proc.h"
#include "_alloc.h"

extern void _proc(pcb_t *);
extern void vector_table(void);

// 쉘 코드
extern uint8_t _task_shell_start[];
extern uint8_t _task_shell_size[];

#pragma endregion

// 커널 함수
void master(void)
{
    kernel_main();
}

void kernel_main(void)
{ // 하드웨어 초기화
    uart_init();
    // 초기화 부분으로 옮김
    init_irq();
    // 동적할당 초기화
    heap_init();

    // 관리자 초기화
    // MM 메타데이터는 MM_ADDR_START에 두고, 실제 프로세스 메모리는 사용자 영역에서 시작한다.
    mm_init(&mm_stack, USER_PROC_START);
    // pm_init(&pm_object, PM_ADDR_START);
    fm_init((uint64_t *)USER_FILE_START);

    FMv3_record *reco = (FMv3_record *)FM_ADDR_START;

    puts("Booting AxKernel\n");

    /*
        파일로 만든뒤 대기 큐에 넣기
    */

    // pcb_t *proc1 = proc_turn(reco, "TA.BIN", &task_inf_A, 0);

    // pcb_t *proc2 = proc_turn(reco, "TB.BIN", &task_inf_B, 0);
    // pm_awake(&pm_object, 0, proc2);
    pcb_t *shell_proc = proc_turn(reco, "SHEL.BIN", _task_shell_start, 1);
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

/*
void init_nic(uint64_t base)
{
    // 1. 장치 리셋
    *(volatile uint32_t *)(base + VIRTIO_MMIO_STATUS) = 0;

    // 2. ACKNOWLEDGE 설정
    uint32_t status = 0;
    status |= VIRTIO_STATUS_ACKNOWLEDGE;
    *(volatile uint32_t *)(base + VIRTIO_MMIO_STATUS) = status;

    // 3. DRIVER 설정
    status |= VIRTIO_STATUS_DRIVER;
    *(volatile uint32_t *)(base + VIRTIO_MMIO_STATUS) = status;

    puts("NIC Status Initialized!\n");
}

void setup_virtqueue(uint64_t base, int queue_index)
{
    *(volatile uint32_t *)(base + VIRTIO_MMIO_QUEUE_SEL) = queue_index;

    *(volatile uint32_t *)(base + VIRTIO_MMIO_QUEUE_NUM) = 128;

    uint32_t pfn = (uint32_t)(((uint64_t)get_ring_buffer_addr() >> 12));
    *(volatile uint32_t *)(base + VIRTIO_MMIO_QUEUE_PFN) = pfn;

    puts("Queue setup done!\n");
}

static unsigned char virtio_ring_buffer[4096] __attribute__((aligned(4096)));

void *get_ring_buffer_addr(void)
{
    return (void *)virtio_ring_buffer;
}

#define VIRTIO_STATUS_DRIVER_OK 4

void debug_main(void)
{
    uint64_t base = 0x0A000000;

    init_nic(base);

    // RX 큐(0번)와 TX 큐(1번)를 각각 등록
    setup_virtqueue(base, 0);
    setup_virtqueue(base, 1);

    uint32_t status = *(volatile uint32_t *)(base + VIRTIO_MMIO_STATUS);
    status |= VIRTIO_STATUS_DRIVER_OK;
    *(volatile uint32_t *)(base + VIRTIO_MMIO_STATUS) = status;

    puts("NIC is fully ready and running!\n");
}
*/