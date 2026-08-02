#ifndef __KERNEL_QUEUE_H__
#define __KERNEL_QUEUE_H__

#include "_types.h"

#define MAX_QUEUE_SIZE 255
/*
typedef struct queue
{
    uint8_t *buffer;

    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t count;

    // 함수
    void (*push)(queue *, uint8_t);
    uint8_t (*pop)(queue *);
    uint8_t (*empty)(queue *);
    uint8_t (*full)(queue *);
} queue;

// 자료구조 초기화 함수
void queue_init(queue *this, uint8_t *buffer, uint32_t size);
*/
#endif
