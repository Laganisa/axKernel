#ifndef __KERNEL_SYSCALL_H__
#define __KERNEL_SYSCALL_H__

#include "_types.h"

// 시스템 콜 슷자

// 일반적인 시스템 콜  (0 ~ 7)
#define SYS_RESERVED0 0
#define SYS_EXIT 1
#define SYS_ABORT 2
#define SYS_BRK 3
#define SYS_YIELD 4
#define SYS_SETUP 5
#define SYS_WRITE 6
#define SYS_READ 7

// File System Call (8 ~ 15)

#define SYS_OPEN 8
#define SYS_CLOSE 9
#define SYS_FILE_CREAT 10
#define SYS_FILE_DEL 11
#define SYS_DIR_CREAT 12
#define SYS_DIR_DEL 13
#define SYS_DISK_LOAD 14
#define SYS_DISK_STORE 15

// Process System Call (16 ~ 23)
// !  프로세스 생명 주기 관련 명령

#define SYS_PROC_CREAT 16
#define SYS_PROC_DEL 17
#define SYS_RESERVED18 18
#define SYS_RESERVED19 19
#define SYS_RESERVED20 20
#define SYS_RESERVED21 21
#define SYS_RESERVED22 22
#define SYS_RESERVED23 23

// IPC System Call (24 ~ 31)

#define SYS_IPC_SEND 24
#define SYS_IPC_RECE 25
#define SYS_RESERVED26 26
#define SYS_RESERVED27 27
#define SYS_RESERVED28 28
#define SYS_RESERVED29 29
#define SYS_RESERVED30 30
#define SYS_RESERVED31 31

// Network System Call (32 ~ 39)

#define SYS_L2_SEND 32
#define SYS_L2_RECE 33
#define SYS_L2_FIND 34
#define SYS_RESERVED35 35
#define SYS_RESERVED36 36
#define SYS_RESERVED37 37
#define SYS_RESERVED38 38
#define SYS_RESERVED39 39

// info system Call (40 ~ 47)

#define SYS_RESERVED40 40
#define SYS_RESERVED41 41
#define SYS_RESERVED42 42
#define SYS_RESERVED43 43
#define SYS_RESERVED44 44
#define SYS_RESERVED45 45
#define SYS_RESERVED46 46
#define SYS_RESERVED47 47

// lib system Call (48 ~ 55)
// ! 생각만 하고 있음

#define SYS_RESERVED48 48
#define SYS_RESERVED49 49
#define SYS_RESERVED50 50
#define SYS_RESERVED51 51
#define SYS_RESERVED52 52
#define SYS_RESERVED53 53
#define SYS_RESERVED54 54
#define SYS_RESERVED55 55

// 아직 만들지 않은 시스템 콜들

#define SYS_RESERVED56 56
#define SYS_RESERVED57 57
#define SYS_RESERVED58 58
#define SYS_RESERVED59 59
#define SYS_RESERVED60 60
#define SYS_RESERVED61 61
#define SYS_RESERVED62 62
#define SYS_RESERVED63 63

#define SYS_RESERVED64 64
#define SYS_RESERVED65 65
#define SYS_RESERVED66 66
#define SYS_RESERVED67 67
#define SYS_RESERVED68 68
#define SYS_RESERVED69 69
#define SYS_RESERVED70 70
#define SYS_RESERVED71 71

#endif