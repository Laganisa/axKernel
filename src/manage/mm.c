// #include "types.h"
#include "io.h"
#include "mm.h"
#include "asm.h"

#include "debug.h"

// ! 수정 사항 존재

/*
    메모리 관리자 초기화 함수
*/
void mm_init(MMv5_stack *stack, uint64_t addr)
{
    stack->base = (uint64_t *)addr; // 스택의 시작 위치 받기
    stack->sp_top = 0x0;            // top 포인터를 시작위치로 옮기기
    stack->sp_bot = 0xFFFF;         // bot 포인터를 마지막 위치로 옮기기
    stack->sp_temp = 0;
    stack->sp = 0;

    for (int i = 0; i < sizeof(stack->MMv5_mem); i++)
        stack->MMv5_mem[i] = 0;
    for (int i = 0; i < sizeof(stack->MMv5_submem); i++)
        stack->MMv5_submem[i] = 0;
}

// ! 메모리 관리자 수정 필요

uint16_t mm_creat(MMv5_stack *stack, uint16_t val16)
{

    // cmd = 0 할당 공간 할당 -> uint16_t 의 스택 포인터 주소 리턴
    uint8_t data = (val16 >> 6) & 0x7; // 7,8,9로 쪼갠 데이터 즉 많아봤자 512KB

    if (data != 0) // 주어진 범위를 초과하지 않았으면
    {
        // ! ufux로 구간 내 비트를 추출하는 로직으로 변경하기
        asm("clz %w0, %w1" : "=r"(data) : "r"((uint32_t)data)); // 어셈블러 따로 처리하는 함수를 만들어야함

        if (stack->sp_top >= stack->sp_bot)
        {
            return 0; // "not enough memory stack"
        }
        else
        {
            // ! 이것도 위에 바꾸면서 바꾸기
            data = 37 - data;
            return MMv5_regu_push(stack, data); // 메모리 스택 주소를 리턴함
        }
    }
    else
    {
        // ! 아직 비정규로 할당할 함수 제작 하지 않음
        // 이 함수는 sp_bot에서 부터 아래로 내려가며 할당할 예정
        // 한번 할당할때 크기는 2비트가 아닌 1바이트 정도로 생각중
    }

    return 0;
}

/*
    가비지 컬랙터를 이용한 공간 해제
    현재 가비지 컬랙터가 보고 있는 스택(4개의 주소가 있는)에
    유효한 주소가 하나라면 uint16_t 주소를 다시 할당하기
    그렇지 않다면
*/
uint8_t mm_free(MMv5_stack *stack, MMv5_stack *substack, uint16_t val16)
{
    // ! 여기에 확인 로직 넣어야함 리턴 인자값도 변경해야함
    uint8_t remain_Byte = MMv5_regu_pop(stack, val16); // 남은 바이트 확인, 이미 기존 값은 없어짐

    if (cnt(remain_Byte & BIT_EVEN8_t) <= 1 && cnt(remain_Byte & BIT_ODD8_t) <= 1) // 비트가 2개가 있는지 확인
    {
        if ((cnt(remain_Byte & 0x0F) ^ cnt(remain_Byte & 0xF0) != 0) || (cnt(remain_Byte & 0x33) != 1)) // 바이트 조각이 하나만 있는지 또는 다 차 있는지확인
        {
            // MMv5 스택 substack에서 푸쉬를 한다.
            if (substack->sp >= 5)
            {
                return 0; // "not enough memory stack"
            }
            else
            {
                return MMv5_regu_substack_push(substack, remain_Byte);
            }
        }
        else
        {
            return 0; //
        }
    }

    return 0;
}

/*
    실제 주소를 리턴하는 함수
    스택 주소로 되어 있는 주소에서 실제 주소를 리턴함
*/
uint64_t mm_find(MMv5_stack *stack, uint16_t val16, uint16_t indi_addr)
{
    // cmd = 2 주소 탐색 -> 프로그램이 주소를 요청하면 uint64_t 를 리턴(간접 주소)
    volatile uint64_t safe_base = (uint64_t)stack->base;

    uint8_t *mem_ptr = (uint8_t *)stack->MMv5_mem;
    uint16_t calculated_val = 0;

    int valQuo = (val16 >> 5);    // 64비트(데이터 32개) 덩어리 개수
    int val_rem = (val16 & 0x1F); // 남은 데이터 개수

    for (int i = 0; i < valQuo; i++)
    {
        uint64_t chunk = 0;
        for (int j = 0; j < 8; j++)
        {
            ((uint8_t *)&chunk)[j] = mem_ptr[(i << 3) + j];
        }

        uint64_t odd = chunk & BIT_ODD64_t;
        uint64_t even = chunk & BIT_EVEN64_t;

        calculated_val += (uint16_t)(cnt(odd) + (cnt(even) << 1));
    }

    if (val_rem > 0)
    {
        uint64_t last_chunk = 0;
        for (int j = 0; j < 8; j++)
        {
            ((uint8_t *)&last_chunk)[j] = mem_ptr[(valQuo << 3) + j];
        }

        uint64_t mask = (1ULL << (val_rem << 1)) - 1;
        uint64_t last_bits = last_chunk & mask;

        calculated_val += (uint16_t)(cnt(last_bits & BIT_ODD64_t) + (cnt(last_bits & BIT_EVEN64_t) << 1));
    }

    return safe_base + ((uint64_t)calculated_val << 10) + indi_addr;
}
