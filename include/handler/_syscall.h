#ifndef __KERNEL_SYSCALL_H__
#define __KERNEL_SYSCALL_H__

#include "_types.h"

// 시스템 콜 슷자

// 일반적인 시스템 콜  (0 ~ 7)
#define SYS_RESERVED0 0
#define SYS_EXIT 1
#define SYS_ABORT 2
#define SYS_LOAD 3
#define SYS_YIELD 4
#define SYS_SETUP 5
#define SYS_WRITE 6
#define SYS_READ 7

// File System Call (8 ~ 15)

#define SYS_FILE_CREAT 8
#define SYS_FILE_DEL 9
#define SYS_OPEN 10
#define SYS_CLOSE 11
#define SYS_RESERVED12 12
#define SYS_DIR_CREAT 13
#define SYS_DIR_DEL 14
#define SYS_RESERVED15 15

// Process System Call (16 ~ 23)

#define SYS_PROC_CREAT 16
#define SYS_PROC_DEL 17
#define SYS_RESERVED18 18
#define SYS_RESERVED19 19
#define SYS_RESERVED20 20
#define SYS_RESERVED21 21
#define SYS_RESERVED22 22
#define SYS_RESERVED23 23

// IPC System Call (24 ~ 31)

#define SYS_RESERVED24 24
#define SYS_RESERVED25 25
#define SYS_RESERVED26 26
#define SYS_RESERVED27 27
#define SYS_RESERVED28 28
#define SYS_RESERVED29 29
#define SYS_RESERVED30 30
#define SYS_RESERVED31 31

// Network System Call (32 ~ 39)

#define SYS_SEND_L2 32
#define SYS_RESERVED33 33
#define SYS_RESERVED34 34
#define SYS_RESERVED35 35
#define SYS_RESERVED36 36
#define SYS_RESERVED37 37
#define SYS_RESERVED38 38
#define SYS_RESERVED39 39

// Syscall handler
uint64_t handle_svc_a64(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

/* 일반적인 시스템 콜 */
int32_t exit_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t write_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t read_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t setup_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);

/* 파일 시스템 콜 */
int32_t open_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t creat_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t close_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t del_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
/* 프로세스 시스템 콜 */

int32_t creat_proc_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
/* 프로세스 간 정보 시스템 콜 */

/* 네트워크 시스템 콜 */
int32_t send_L2_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);

#endif