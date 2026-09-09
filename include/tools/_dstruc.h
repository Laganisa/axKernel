#ifndef __KERNEL_QUEUE_H__
#define __KERNEL_QUEUE_H__

#include "_types.h"

#define MAX_QUEUE_SIZE 255

typedef struct queue
{
    uint8_t *buffer;

    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t count;

    // 함수
    void (*push)(struct queue *, uint8_t);
    uint8_t (*pop)(struct queue *);
    uint8_t (*empty)(struct queue *);
    uint8_t (*full)(struct queue *);
} queue;

// 자료구조 초기화 함수
void queue_init(queue *this, uint8_t *buffer, uint32_t size);

typedef struct stack
{
    uint8_t *buffer;

    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t count;

    // 함수
    void (*push)(struct queue *, uint8_t);
    uint8_t (*pop)(struct queue *);
    uint8_t (*empty)(struct queue *);
    uint8_t (*full)(struct queue *);
} stack;

// 자료구조 초기화 함수
void stack_init(queue *this, uint8_t *buffer, uint32_t size);

#endif
