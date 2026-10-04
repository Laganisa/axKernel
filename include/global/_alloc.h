#ifndef __KERNEL_ALLOC_H__
#define __KERNEL_ALLOC_H__

#include "_types.h"

/// @brief 힙 블록 구조체
typedef struct heap_block_t
{
    uint32_t size;             // 블록 크기 (헤더 포함)
    uint32_t is_alloc : 1;     // 할당 여부 (1: 할당됨, 0: 해제됨)
    uint32_t padding : 31;     // 정렬을 위한 패딩
    struct heap_block_t *next; // DLL의 다음 블록 포인터
    struct heap_block_t *prev; // DLL의 이전 블록 포인터

} heap_block_t;

/// @brief 힙 초기화 함수
/// @param void
void heap_init(void);

/// @brief
/// @param size
/// @return
void *heap_alloc(uint32_t size);

/// @brief
/// @param ptr
void heap_free(void *ptr);

#endif