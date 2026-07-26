#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    가상 큐 관련 파일
*/

static unsigned char rx_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static unsigned char tx_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));

void *get_ring_buffer_addr(void)
{
    return (void *)tx_queue.storage;
}

struct virtio_queue_state rx_queue;
struct virtio_queue_state tx_queue;

uint16_t last_rx_used_idx = 0;
uint16_t last_tx_used_idx = 0;

static void setup_queue_state(int queue_index, struct virtio_queue_state *queue)
{
    VIRTIO_QUEUE_SEL = queue_index;

    uint32_t max = VIRTIO_QUEUE_NUM_MAX;

    dump("Queue max supported: ", max);

    if (max == 0)
    {
        puts("ERROR: Queue not available!\n");
        return;
    }

    if (max < VIRTIO_QUEUE_SIZE)
    {
        puts("ERROR: Queue is smaller than the driver ring!\n");
        return;
    }

    VIRTIO_GUEST_PAGE_SIZE = 4096;
    VIRTIO_QUEUE_ALIGN = 4096;
    VIRTIO_QUEUE_NUM = VIRTIO_QUEUE_SIZE;

    queue->desc = (struct virtq_desc *)queue->storage;
    queue->avail = (struct virtq_avail *)(queue->storage + VIRTIO_DESC_BYTES);
    queue->used = (struct virtq_used *)(queue->storage + VIRTIO_USED_OFFSET);

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        queue->storage[i] = 0;
    }

    VIRTIO_QUEUE_PFN = ((uint64_t)queue->storage) >> 12;
    dump("Queue PFN set: ", VIRTIO_QUEUE_PFN);
}

void setup_virtqueue(int queue_index)
{
    if (queue_index == 0)
    {
        rx_queue.storage = rx_queue_storage;
        setup_queue_state(0, &rx_queue);
    }
    else
    {
        tx_queue.storage = tx_queue_storage;
        setup_queue_state(1, &tx_queue);
    }
    puts("Queue setup done with PFN!\n");
}