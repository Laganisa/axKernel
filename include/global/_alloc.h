#ifndef __KERNEL_ALLOC_H__
#define __KERNEL_ALLOC_H__

#include "_types.h"

typedef struct heap_block_t
{
    uint32_t size;         // 블록 크기 (헤더 포함)
    uint32_t is_alloc : 1; // 할당 여부
    uint32_t reserved : 31;
    struct heap_block_t *next; // 다음 블록
    struct heap_block_t *prev; // 이전 블록
} heap_block_t;

void heap_init(void);
void *heap_alloc(uint32_t size);
void heap_free(void *ptr);

#endif