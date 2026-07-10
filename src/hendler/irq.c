#include "io.h"
#include "sync.h"

#include "mm.h"

#include "irq.h"

#include "debug.h"
#include "meta.h"
// extern void vector_table(void);

// 시스템 타이머: 두 타이머 인터럽트 간의 시간을 tick으로 나타낸거
static uint64_t system_tick = 0;

pcb_t *current_proc = 0;

pcb_t *get_current_proc_addr()
{
    return current_proc;
}

// 타이머 켜지면 틱 값을 올리기
pcb_t *irq_handler_main(pcb_t *proc)
{
    disable_irq();

    // ! 장치 관리자 만들기
    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    uint32_t irq_nr = iar & 0x3FF;

    if (irq_nr == 30)
    {
        current_proc = schedule_proc(proc);
    }

    *(volatile uint32_t *)(GIC_CPU_BASE + 0x10) = iar;

    // 타이머 재설정
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));

    enable_irq();
    return current_proc;
}