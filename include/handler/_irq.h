#ifndef __KERNEL_IRQ_H__
#define __KERNEL_IRQ_H__

#include "manage/_pm.h"

pcb_t *irq_handler_main(pcb_t *proc);

void handle_timer_tick();
void init_vectors();

void init_timer();

void init_gic();

void init_irq();

#endif