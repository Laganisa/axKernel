#ifndef __KERNEL_GIC_H__
#define __KERNEL_GIC_H__
#include "_types.h"

#define VIRTIO_IRQ 32

void gic_init(void);

void gic_enable_irq(uint32_t irq);
void gic_disable_irq(uint32_t irq);

void gic_set_priority(uint32_t irq, uint8_t priority);

#endif