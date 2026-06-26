#ifndef __DEFS_H__
#define __DEFS_H__

// 인터럽트 관련
#define enable_irq() asm volatile("msr daifclr, #2")
#define disable_irq() asm volatile("msr daifset, #2")

// 산수 관련

#define abs(x) ((x) < 0 ? -(x) : (x))

// 계산 관련
#define BIT_ODD8_t 0x55  // 8비트 홀수 비트 검정
#define BIT_EVEN8_t 0xAA // 8비트 짝수 비트 검정

#define BIT_ODD64_t 0x5555555555555555  // 64비트 홀수 비트 검정
#define BIT_EVEN64_t 0xAAAAAAAAAAAAAAAA // 64비트 짝수 비트 검정

// 프로세스 관련
#pragma region Proc

#define MAX_PCB_SIZE 0x100     // 최대 pcb 수 256개 0은 pid 0 운영체제 꺼
#define MAX_PCB_BITSIZE 0x04   // 최대 pcb bit 수 4개 이유는 1비트당 하나의 프로세서이니까
#define INITIAL_PROC_SIZE 0x80 // 초기 프로세스 할당 크기
#define PROC_SIGNAL 0xFF       // low q를 확인할 차례라는 시그널
#define PMV1_MAX_PROC 0xFF     // 프로세스 최대 수
#define MAX_PTP_MSGBOX 0x10    // 프로세스간 메시지 통신 개수 제한, 이유 : 이러면 딱 1KB
#define PROC_DORM 0x1          // 프로세스 휴면 상태
#define PROC_STOP 0x2          // 프로세스 휴면 상태
#define PROC_ZOMB 0x3          // 프로세스 휴면 상태
#define INIT_PROC_SECT 1       // 큐에서 아무 것도 없을때 꺼내는 값을 정하는거

#pragma endregion

// 파일 관리자 관련
#pragma region file

#define MAX_FILE_NAME 8 // 최대 파일 이름 길이

// V2 파일 관리자 - 1MB 파일 지원
#define MAX_FILE_SIZE 0x100000 // 최대 파일 크기: 1MB
#define MAX_DIR_SIZE 0x200000  // 디렉토리당 최대 크기: 2MB
#define MAX_FCB_file 0x10      // FCB 최대 수 (16개)
#define MAX_FCB_dir 0x04       // 디렉토리 최대 수 4개

#pragma endregion

#endif
