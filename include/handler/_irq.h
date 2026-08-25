#ifndef __KERNEL_IRQ_H__
#define __KERNEL_IRQ_H__

#include "manage/_pm.h"
#include "handler/_gic.h"

void irq_handler_main(pcb_t *proc);

// pcb_t *irq_handler_main(pcb_t *proc);

void vector_init(void);

void timer_init(void);

void irq_init(void);

#endif