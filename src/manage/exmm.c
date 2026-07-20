#include "global/_io.h"
#include "manage/_mm.h"
#include "tools/_asm.h"
#include "global/_debug.h"

/*
    정규 스택 푸쉬 함수

    메모리 관리자 스택 연산 push
    retrun 0: fail
    else 주소
*/
uint8_t MMv5_regu_push(MMv5_stack *stack, uint8_t val)
{
    // 할당은 항상 sp_top에서 하고
    uint16_t result;
    // ?  bot 포인터와 겹치지 않는지 확인이 필요함
    uint16_t byte_idx = stack->sp_top >> 2;     // 몇 번째 바이트인지 (0~16383)
    uint8_t bit_pos = (stack->sp_top & 3) << 1; // 바이트 내 비트 위치 (0, 2, 4, 6)

    // ! 기존 비트 청소 후 새 2비트 값 삽입
    stack->MMv5_mem[byte_idx] &= ~(0x03 << bit_pos);
    stack->MMv5_mem[byte_idx] |= (val & 0x03) << bit_pos;
    result = stack->sp_top;
    stack->sp_top++; // 16비트 포인터 증가!
    return result;   // 할당된 위치 알려주기
}

/*
    비 정규 스택 푸쉬 함수
    비트는 3비트
*/
uint8_t MMv5_irregu_push(MMv5_stack *stack, uint8_t val)
{
    /*
    // 할당은 항상 sp_top에서 하고
    uint16_t result;
    // ?  bot 포인터와 겹치지 않는지 확인이 필요함
    uint16_t byte_idx = stack->sp_top >> 2;     // 몇 번째 바이트인지 (0~16383)
    uint8_t bit_pos = (stack->sp_top & 3) << 1; // 바이트 내 비트 위치 (0, 2, 4, 6)

    // ! 기존 비트 청소 후 새 2비트 값 삽입
    stack->MMv5_mem[byte_idx] &= ~(0x03 << bit_pos);
    stack->MMv5_mem[byte_idx] |= (val & 0x03) << bit_pos;
    result = stack->sp_top;
    stack->sp_top++; // 16비트 포인터 증가!
    */

    // return result; // 할당된 위치 알려주기
}

// 서브스택의 푸쉬(가비지 컬랙터)
uint8_t MMv5_regu_substack_push(MMv5_stack *stack, uint8_t val)
{
    stack->MMv5_submem[stack->sp] = val;
    stack->sp++;
}

/*
    // 메모리_관리자 스택 연산 pop
    // ! 수정 사항 존재
*/
uint8_t MMv5_regu_pop(MMv5_stack *stack, uint16_t val)
{
    // 해제는 val 값을 제거
    if (stack->sp_top == 0)
    {
        return 0xFF; // Stack Underflow
    }

    // ! 아래 로직 바꾸기 필수
    stack->sp_temp = val; // 덤프 포인터를 val의 위치로 옮기고 0으로 포팅
    uint16_t byte_idx = stack->sp_temp >> 2;
    uint8_t bit_pos = (stack->sp_temp & 3) << 1;

    return stack->MMv5_mem[byte_idx]; // 스택의 덤프 포인터가 가리키는 uint8_t 을 리턴
}

/*
    비 정규로 할당하는 함수 팝
*/
uint8_t MMv5_irregu_pop(MMv5_stack *stack, uint16_t val)
{
    // 해제는 val 값을 제거
    if (stack->sp_top == 0)
    {
        return 0xFF; // Stack Underflow
    }

    // ! 아래 로직 바꾸기 필수
    stack->sp_temp = val; // 덤프 포인터를 val의 위치로 옮기고 0으로 포팅
    uint16_t byte_idx = stack->sp_temp >> 2;
    uint8_t bit_pos = (stack->sp_temp & 3) << 1;

    return stack->MMv5_mem[byte_idx]; // 스택의 덤프 포인터가 가리키는 uint8_t 을 리턴
}