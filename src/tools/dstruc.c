#include "tools/_dstruc.h"

static void queue_push(queue *this, uint8_t data)
{
    if (this->count >= this->size)
        return;

    this->buffer[this->head] = data;

    this->head++;
    this->count++;

    if (this->head >= this->size)
        this->head = 0;
}

static uint8_t queue_pop(queue *this)
{
    if (this->count == 0)
        return 0;

    uint8_t data = this->buffer[this->tail];

    this->tail++;
    this->count--;

    if (this->tail >= this->size)
        this->tail = 0;

    return data;
}

static uint8_t queue_empty(queue *this)
{
    return this->count == 0;
}

static uint8_t queue_full(queue *this)
{
    return this->count >= this->size;
}

void queue_init(queue *this, uint8_t *buffer, uint32_t size)
{
    this->buffer = buffer;

    this->size = size;

    this->head = 0;
    this->tail = 0;
    this->count = 0;

    this->push = queue_push;
    this->pop = queue_pop;
    this->empty = queue_empty;
    this->full = queue_full;
}
