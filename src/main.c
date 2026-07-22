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

void devo_main(void)
{

#define VIRTIO_MMIO_BASE 0x0a000000UL
#define VIRTIO_SLOT_SIZE 0x200 // 슬롯 간격
#define MAX_VIRTIO_SLOTS 32    // 검사할 슬롯 개수

#define VIRTIO_MMIO_MAGIC_VALUE 0x00
#define VIRTIO_MMIO_DEVICE_ID 0x08

    puts("[axOS] Scanning Virtio MMIO slots...\n");

    for (int i = 0; i < MAX_VIRTIO_SLOTS; i++)
    {
        uint64_t addr = VIRTIO_MMIO_BASE + (i * VIRTIO_SLOT_SIZE);

        // 1. Magic Value 확인 ("virt" -> 0x74726976)
        uint32_t magic = *(volatile uint32_t *)(addr + VIRTIO_MMIO_MAGIC_VALUE);
        if (magic != 0x74726976)
        {
            continue;
        }

        // 2. Device ID 확인
        uint32_t device_id = *(volatile uint32_t *)(addr + VIRTIO_MMIO_DEVICE_ID);
        if (device_id == 0)
        {
            continue;
        }

        // 3. 주소랑 디바이스 ID 출력
        puts("Found Device at Addr: ");
        put_hex(addr);
        puts(" | Device ID: ");
        put_hex(device_id);
        puts("\n");

        // Virtio-GPU의 Device ID는 16이야!
        if (device_id == 16)
        {
            puts(" -> [Bingo!] Virtio-GPU found here!\n");
            // TODO: 여기서 이 addr를 GPU 베이스 주소로 저장하면 됨!
        }
    }
    puts("[axOS] Scan finished.\n");
}