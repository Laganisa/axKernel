#include "../include/types.h"

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// 시스템 콜 슷자

#pragma region pm

#define SYS_EXIT 1
#define sys_ABORT 2
#define sys_LOAD 3
#define SYS_YIELD 4
#define SYS_PROC_CRATE 5

#pragma endregion

#pragma region fm

#define SYS_WRITE 6
#define SYS_READ 7
#define SYS_FN_CRATE 8
#define SYT_FM_DELETE 9
#define SYS_OPEN 10
#define SYS_CLOSE 11

#pragma endregion

#pragma region etc

// 파이프는 부모 자식 간의 관계에만 사용됨
#define SYS_PIPE 12  // 파이프 생성
#define SYS_NPIPE 13 // 파이프 해제
#define SYS_CHMOD 14 // 파일 권한 변경

// Syscall handler
uint64_t handle_syscall(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);
#endif
