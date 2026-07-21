#ifndef __KERNEL_IN_PROC_H__
#define __KERNEL_IN_PROC_H__

#include "_types.h"

void ROOT(void);
void INIT(void);

void temp_posi(void);
void task_wfi(void);
void task_hang(void);

// 디버깅용 프로세스
void task_inf_A(void);
void task_inf_B(void);
void task_stop_B(void);

#endif