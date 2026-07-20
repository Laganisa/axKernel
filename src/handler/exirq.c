#include "global/_io.h"
#include "handler/_sync.h"
#include "manage/_mm.h"
#include "handler/_irq.h"
#include "global/_debug.h"
#include "global/_meta.h"

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

// ! 디버그 바꾸기
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
    // 1. Distributor OFF
    GIC_DIST_CTRL = 0;

    // 2. 모든 인터럽트를 Group 0으로 설정
    GIC_DIST_IGROUPR0 = 0x00000000;

    // 3. 모든 PPI(16~31) 우선순위 설정
    for (int i = 16; i < 32; i++)
    {
        GIC_DIST_REG8(GIC_DIST_PRIORITY + i) = 0x80;
    }

    // 모든 PPI enable
    GIC_DIST_ENABLE0 = 0xFFFF0000;

    // IRQ 30 우선순위 설정
    GIC_DIST_REG8(GIC_DIST_PRIORITY + 30) = 0xA0;

    // IRQ 30 enable
    GIC_DIST_ENABLE0 |= (1U << 30);

    // 4. Distributor enable
    GIC_DIST_CTRL = 1;

    // 5. CPU Interface enable
    GIC_CPU_CTRL = 1;
    asm volatile("dsb sy");
    asm volatile("isb");

    // Priority mask 설정
    GIC_CPU_PMR = 0xFF;
    asm volatile("dsb sy");

    // IRQ unmask
    enable_irq();
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
    enable_irq();

    // SGI (Software Generated Interrupt) 발생
    *(volatile uint32_t *)(GIC_DIST_BASE + 0xF00) = 0x8000 | 0; // GICD_SGIR

    disable_irq();
}