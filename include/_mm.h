#include "_defs.h"
#include "_types.h"

#ifndef __MM_H__
#define __MM_H__

// 메모리 관리자가 담당하는 메모리 선언
typedef struct MMv5_stack
{
    uint64_t *base;          // mmv5의 스택의 바닥 주소
    uint16_t sp_top;         // mmv5의 top 스택 포인터 (정규적인 공간 할당)
    uint16_t sp_bot;         // mmv5의 bottom 스택 포인터(비정규적인 공간 할당을 맡음)
    uint16_t sp_temp;        // 해제에 사용될 포인터
    uint8_t MMv5_mem[16384]; // 스택 크기 16KB (위로 자라는 스택)

    uint8_t sp;             // 서브 스택 포인터
    uint8_t MMv5_submem[5]; // 4개면 되는데 예방차원으로 5개로 부여

} __attribute__((aligned(8))) MMv5_stack;

// 함수 선언
void mm_init(MMv5_stack *stack, uint64_t addr);
uint8_t MMv5_regu_push(MMv5_stack *stack, uint8_t val);
uint8_t MMv5_regu_substack_push(MMv5_stack *stack, uint8_t val);
uint8_t MMv5_regu_pop(MMv5_stack *stack, uint16_t val);

// 분리함
// void *mm_run(MMv5_stack *stack, MMv5_stack *substack, int8_t cmd, uint16_t val16, uint16_t indi_addr);

// 할당 공간 할당 -> uint16_t 의 스택 포인터 주소 리턴
uint16_t mm_creat(MMv5_stack *stack, uint16_t val16);
// 해제 공간 해제 -> 그 보는 바이트에 주소가 하나라면 uint16_t 의 주소 재할당
uint8_t mm_free(MMv5_stack *stack, MMv5_stack *substack, uint16_t val16);
// 주소 탐색 -> 프로그램이 주소를 요청하면 uint64_t 를 리턴(간접 주소)
uint64_t mm_find(MMv5_stack *stack, uint16_t val16, uint16_t indi_addr);

// 전역 구조체 선언
#define mm_stack (*(MMv5_stack *)MM_ADDR_START)
#define mm_substack mm_stack

#endif
