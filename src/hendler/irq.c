#include "io.h"
#include "sync.h"

#include "mm.h"

#include "irq.h"

#include "debug.h"
#include "meta.h"
extern void vector_table(void);

// extern volatile uint8_t resched_flag;

pcb_t *current_proc = 0;

pcb_t *get_current_proc_addr()
{
    return current_proc;
}

pcb_t *irq_handler_main(pcb_t *proc)
{
    disable_irq();
    enter("irq_handler_main");

    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    uint32_t irq_nr = iar & 0x3FF;

    if (irq_nr == 30)
    {
        current_proc = schedule_proc(proc);
    }

    *(volatile uint32_t *)(GIC_CPU_BASE + 0x10) = iar;

    // 타이머 재설정
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));

    dump("proc", current_proc);
    exit("irq_handler_main");
    enable_irq();

    return current_proc;
}