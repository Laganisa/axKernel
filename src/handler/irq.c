#include "global/_io.h"
#include "handler/_sync.h"
#include "handler/_irq.h"
#include "manage/_dm.h"
#include "global/_debug.h"
#include "_defs.h"
#include "global/_meta.h"

// 시스템 타이머: 두 타이머 인터럽트 간의 시간을 tick으로 나타낸거
static uint64_t system_tick = 0;

pcb_t *current_proc = 0;

pcb_t *get_current_proc_addr()
{
    return current_proc;
}

// 타이머 인터럽트가 발생하면 틱 값을 올리고 스케줄링
pcb_t *irq_handler_main(pcb_t *proc)
{
    /*
    disable_irq();

    uint32_t iar = GIC_CPU_IAR;
    uint32_t irq = iar & 0x3FF;

    switch (irq)
    {
    case NSPTI:

        // 타이머 인터럽트
        current_proc = schedule_proc(proc);

        // 타이머 재설정
        asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));

        break;

    case VIRTIO_IRQ:

        current_proc = schedule_proc(proc);

        break;

    default:

        dump("Unknown IRQ", irq);

        break;
    }

    GIC_CPU_EOI = iar;

    enable_irq();

    return current_proc;
    */

    disable_irq(); // 인터럽트 번호 읽기
    uint32_t iar = GIC_CPU_IAR;
    uint32_t irq_nr = iar & 0x3FF;

    if (irq_nr == NSPTI)
    {
        current_proc = schedule_proc(proc);
    }

    // 인터럽트 처리 완료 알림
    GIC_CPU_EOI = iar;

    // 타이머 재설정
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));
    enable_irq();
    return current_proc;
}
