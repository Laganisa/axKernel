#include "../include/pm.h"

#ifndef __IRQ_H__
#define __IRQ_H__

pcb_t *irq_handler_main(pcb_t *proc);

void handle_timer_tick();
void init_vectors();

void init_timer();

void init_gic();

void init_irq();

#endif