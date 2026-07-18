#include "_alloc.h"

/*
    Linked List 기반 동적할당 구현

    메모리 구조:
    [heap_block_t 헤더][데이터]...[heap_block_t 헤더][데이터]...

    각 블록은 linked list로 연결되어 있으며,
    free 블록을 찾아서 할당하거나 필요시 분할(split)함
*/

#define HEAP_SIZE (1024 * 100) // 100KB
static uint8_t heap_mem[HEAP_SIZE] = {0};
static heap_block_t *heap_head = NULL;

// 힙 초기화: 전체를 하나의 큰 Free 블록으로 설정
void heap_init(void)
{
    heap_head = (heap_block_t *)heap_mem;
    heap_head->size = HEAP_SIZE;
    heap_head->is_alloc = 0;
    heap_head->next = NULL;
    heap_head->prev = NULL;
}

// 주소에서 헤더를 역추적하여 반환
static heap_block_t *get_header(void *ptr)
{
    if (ptr == NULL)
        return NULL;
    return (heap_block_t *)ptr - 1;
}

// 힙에서 메모리 할당
// First-fit 알고리즘 사용
void *heap_alloc(uint32_t size)
{
    uint32_t total_size = size + sizeof(heap_block_t);
    heap_block_t *current = heap_head;

    while (current != NULL)
    {
        // Free 블록이고 충분한 크기인지 확인
        if (!current->is_alloc && current->size >= total_size)
        {
            // 분할 필요 여부 확인
            if (current->size > total_size + sizeof(heap_block_t))
            {
                // 남은 공간이 있으면 분할
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)current + total_size);
                new_block->size = current->size - total_size;
                new_block->is_alloc = 0;
                new_block->next = current->next;
                new_block->prev = current;

                if (current->next != NULL)
                    current->next->prev = new_block;

                current->next = new_block;
                current->size = total_size;
            }

            // 현재 블록을 할당된 것으로 표시
            current->is_alloc = 1;

            // 데이터 영역 반환 (헤더 다음)
            return (void *)(current + 1);
        }

        current = current->next;
    }

    return NULL; // 할당 실패
}

// 메모리 해제
void heap_free(void *ptr)
{
    if (ptr == NULL)
        return;

    heap_block_t *block = get_header(ptr);
    block->is_alloc = 0;

    // 인접한 Free 블록과 병합 (Coalescing)
    // 다음 블록과 병합
    if (block->next != NULL && !block->next->is_alloc)
    {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next != NULL)
            block->next->prev = block;
    }

    // 이전 블록과 병합
    if (block->prev != NULL && !block->prev->is_alloc)
    {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next != NULL)
            block->next->prev = block->prev;
    }
}
