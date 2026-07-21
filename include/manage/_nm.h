#ifndef __KERNEL_NM_H__
#define __KERNEL_NM_H__

#include "_defs.h"
#include "_types.h"
#include "_macro.h"
#include "manage/_dm.h"

void *get_ring_buffer_addr(void);

void setup_virtqueue(int queue_index);

void net_send_test(void);
void setup_virtqueue(int queue_index);
void check_nic_completion(void);

void debug_main(void);

#endif