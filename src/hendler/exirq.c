#include "_io.h"
#include "_sync.h"

#include "_mm.h"

#include "_irq.h"

#include "_debug.h"
#include "_meta.h"
extern void vector_table(void);

// 이거 왜 있음?
void handle_timer_tick()
{
    log("!\n");
    // 여기서 나중에 스케줄러를 호출해서 current_pcb_addr를 task_B로 바꾸자
}

void init_vectors()
{
    // VBAR_EL1 레지스터에 테이블의 시작 주소를 대입
    asm volatile("msr vbar_el1, %0" : : "r"(vector_table));

    // ISB 추가 (명령어 동기화)
    asm volatile("isb");

    uint64_t check;
    asm volatile("mrs %0, vbar_el1" : "=r"(check));
}

// 디버그 바꾸기
void init_timer()
{
    uint32_t freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));

    // CNTP는 CNTHCTL_EL2[0]=1로만 되면 작동함
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000)); // 0.1초
    asm volatile("msr cntp_ctl_el0, %0" : : "r"(1));          // Enable=1, IMASK=0

    // dump("cntp_ctl_el0", freq);
}

void init_gic()
{

    *(volatile uint32_t *)(GIC_DIST_BASE + 0x000) = 0;
    // puts("[GIC] Distributor OFF\n");

    *(volatile uint32_t *)(GIC_DIST_BASE + 0x080) = 0x00000000;

    // 모든 PPI 인터럽트의 우선순위 설정
    // 각 인터럽트당 1 바이트
    for (int i = 16; i < 32; i++)
    {
        *(volatile uint8_t *)((uintptr_t)GIC_DIST_BASE + 0x400 + i) = 0x80;
    }
    // puts("[GIC] Priority set for all PPI\n");

    // 모든 PPI 인터럽트 enable (ISENABLER0)
    // 비트 16-31 = PPI 16-31
    *(volatile uint32_t *)(GIC_DIST_BASE + 0x100) = 0xFFFF0000; // PPI를 모두 활성화
    uint32_t check = *(volatile uint32_t *)(GIC_DIST_BASE + 0x100);
    // puts("[GIC] ISENABLER0 set to: ");
    // put_hex(check);
    // puts("\n");

    *(volatile uint32_t *)(GIC_DIST_BASE + 0x000) = 0;

    *(volatile uint32_t *)(GIC_DIST_BASE + 0x400 + 30) = 0xA0; // 우선순위

    *(volatile uint32_t *)(GIC_DIST_BASE + 0x100) |= (1 << 30); // 인터럽트

    // 4. distributor enable
    *(volatile uint32_t *)(GIC_DIST_BASE + 0x000) = 1;

    // 5. CPU interface enable
    *(volatile uint32_t *)(GIC_CPU_BASE + 0x000) = 1;
    asm volatile("dsb sy");
    asm volatile("isb");

    *(volatile uint32_t *)(GIC_CPU_BASE + 0x004) = 0xFF;
    asm volatile("dsb sy");

    // IRQ unmask
    asm volatile("msr daifclr, #2");
}

void init_irq()
{

    // 벡터 테이블 등록
    init_vectors();

    // VBAR 확인
    // reg_vbar();

    // GIC 초기화
    init_gic();

    // 타이머 설정
    init_timer();

    // CPU의 인터럽트를 허용
    asm volatile("msr daifclr, #2");
    // puts("[IRQ] DAIF IRQ unmasked\n");

    // 소프트웨어에서 강제로 인터럽트 발생
    // puts("[TEST] Forcing software interrupt...\n");
    // asm volatile("msr daifset, #2"); // 잠시 IRQ 마스크

    // SGI (Software Generated Interrupt) 발생
    *(volatile uint32_t *)(GIC_DIST_BASE + 0xF00) = 0x8000 | 0; // GICD_SGIR

    asm volatile("msr daifclr, #2"); // IRQ 언마스크
    // puts("[TEST] After forced interrupt\n");
}