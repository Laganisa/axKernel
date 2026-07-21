#ifndef __KERNEL_DEVICE_H__
#define __KERNEL_DEVICE_H__

#include "manage/_dm.h"
#include "_macro.h"

int uart_dev_write(void *buf);
int uart_dev_read(void *buf);
void uart_dev_init(void);

void nic_dev_init(void);

#endif