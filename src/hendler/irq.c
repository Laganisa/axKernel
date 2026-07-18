#include "_io.h"
#include "_sync.h"
#include "_irq.h"
#include "_dm.h"
#include "_debug.h"
#include "_defs.h"

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

    uint32_t iar = *GICC_IAR;
    uint32_t irq_nr = iar & 0x3FF;

    // uint32_t irq_nr = check_glc();
    if (irq_nr == NSPTI)
    {
        current_proc = schedule_proc(proc);
    }

    // *(volatile uint32_t *)(GIC_CPU_BASE + 0x10) = iar;
    *GICC_EOI = iar;

    // 타이머 재설정
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));

    enable_irq();
    return current_proc;
}
