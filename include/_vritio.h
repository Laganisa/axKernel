
#ifndef __KERNEL_VRITIO_H__
#define __KERNEL_VRITIO_H__

#include "_types.h"

struct virtq_desc
{
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct virtq_avail
{
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[256];
};

struct virtq_used_elem
{
    uint32_t id;
    uint32_t len;
};

struct virtq_used
{
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[256];
};

typedef struct virtio_queue_state
{
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
    unsigned char *storage;
} virtio_queue_state;

extern virtio_queue_state rx_queue;
extern virtio_queue_state tx_queue;

#define VIRTIO_QUEUE_SIZE 256

#define VIRTIO_DESC_BYTES (16 * VIRTIO_QUEUE_SIZE)
#define VIRTIO_AVAIL_BYTES (4 + 2 * VIRTIO_QUEUE_SIZE)
#define VIRTIO_USED_BYTES (4 + 8 * VIRTIO_QUEUE_SIZE)

#define ALIGN_4K(val) (((val) + 4095) & ~4095)

#define VIRTIO_USED_OFFSET ALIGN_4K(VIRTIO_DESC_BYTES + VIRTIO_AVAIL_BYTES)
#define VIRTIO_QUEUE_BYTES (VIRTIO_USED_OFFSET + VIRTIO_USED_BYTES)
#define VIRTIO_QUEUE_STORAGE ALIGN_4K(VIRTIO_QUEUE_BYTES)

extern uint16_t last_rx_used_idx;
extern uint16_t last_tx_used_idx;

static inline void virtio_mb(void)
{
    __asm__ volatile("dmb ishst" ::: "memory");
}

void *get_ring_buffer_addr(void);

void setup_virtqueue(int queue_index);

#endif