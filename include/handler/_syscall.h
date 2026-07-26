#ifndef __KERNEL_SYSCALL_H__
#define __KERNEL_SYSCALL_H__

#include "_types.h"

// 시스템 콜 슷자

// 일반적인 시스템 콜

#define SYS_EXIT 1
#define SYS_ABORT 2
#define SYS_LOAD 3
#define SYS_YIELD 4
#define SYS_WRITE 6
#define SYS_READ 7
#define SYS_OPEN 10
#define SYS_CLOSE 11

// 파일 시스템 콜
#define SYS_FILE_CREAT 8
#define SYS_FILE_DEL 9
#define SYS_DIR_CREAT 13
#define SYS_DIR_DEL 14

// 프로세스 시스템 콜
#define SYS_PROC_CRATE 5
#define SYS_PROC_DEL 12

// Syscall handler
uint64_t handle_svc_a64(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

int32_t exit_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t write_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t read_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t open_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t creat_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t close_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t del_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);

#endif