#include "global/_io.h"
#include "handler/_sync.h"
#include "handler/_gic.h"

extern void vector_table(void);

void vector_init(void)
{
    asm volatile(
        "msr vbar_el1, %0"
        :
        : "r"(vector_table));

    asm volatile("isb");
}

void timer_init(void)
{
    asm volatile(
        "msr cntp_tval_el0, %0"
        :
        : "r"(0x1000000));

    asm volatile(
        "msr cntp_ctl_el0, %0"
        :
        : "r"(1));
}

void irq_init(void)
{
    vector_init();

    gic_init();

    gic_set_priority(NSPTI, 0xA0);
    gic_enable_irq(NSPTI);

    gic_set_priority(VIRTIO_IRQ, 0x80);
    gic_enable_irq(VIRTIO_IRQ);

    timer_init();

    enable_irq();
}