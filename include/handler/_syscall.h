#include "_types.h"

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// 시스템 콜 슷자

#pragma region pm

#define SYS_EXIT 1
#define SYS_ABORT 2
#define SYS_LOAD 3
#define SYS_YIELD 4
#define SYS_PROC_CRATE 5
#define SYS_PROC_CRATE 15

#pragma endregion

#pragma region fm

#define SYS_WRITE 6
#define SYS_READ 7
#define SYS_FILE_CRATE 8
#define SYS_FILE_DELETE 9
#define SYS_OPEN 10
#define SYS_CLOSE 11

#pragma endregion

#pragma region etc

// 파이프는 부모 자식 간의 관계에만 사용됨
#define SYS_PIPE 12  // 파이프 생성
#define SYS_NPIPE 13 // 파이프 해제
#define SYS_CHMOD 14 // 파일 권한 변경

// Syscall handler
uint64_t handle_svc_a64(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

int32_t exit_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t write_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t read_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t open_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t creat_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);
int32_t close_call(uint64_t arg1, uint64_t arg2, uint64_t arg3);

#endif