#include "global/_io.h"
#include "handler/_sync.h"
#include "handler/_irq.h"
#include "manage/_dm.h"
#include "global/_debug.h"
#include "_defs.h"
#include "global/_meta.h"
#include "global/_dtb.h"

extern void _proc(pcb_t *);
extern uint32_t g_virtio_net_irq;

// 시스템 타이머: 두 타이머 인터럽트 간의 시간을 tick으로 나타낸거
static uint64_t system_tick = 0;

pcb_t *current_proc = 0;

pcb_t *get_current_proc_addr()
{
    return current_proc;
}

// 타이머 인터럽트가 발생하면 틱 값을 올리고 스케줄링
void irq_handler_main(pcb_t *proc)
{
    enter("irq_handler_main");

    disable_irq();

    // 인터럽트 번호 읽기
    uint32_t iar = GIC_CPU_IAR;
    uint32_t irq_nr = iar & 0x3FF;

    switch (irq_nr)
    {
    // 타이머 인터럽트
    case NSPTI:

        current_proc = schedule_proc(proc);

        asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));

        GIC_CPU_EOI = iar;
        enable_irq();

        _proc(current_proc);
    case 79:
        dump_("IRQ NUM", irq_nr);

        // 그대로 전환 하기
        _proc(proc);
    // 맞지 않으면?
    default:
        dump("Unknown IRQ", irq_nr);
        full_stop();
        break;
    }
}
