#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    가상 큐 mmio를 이용한 네트워크 코드
*/

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

/*
static void net_setup_queue(int queue_index, struct virtio_queue_state *queue)
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
        // rx_queue.storage = rx_queue_storage;
        net_setup_queue(0, &rx_queue);
    }
    else
    {
        tx_queue.storage = tx_queue_storage;
        net_setup_queue(1, &tx_queue);
    }
    puts("Queue setup done with PFN!\n");
}
*/

void net_TX_main(void)
{

    vq_init(g_virtio_net_base);

    vq_setup(g_virtio_net_base, 1, tx_queue_storage, &tx_queue);

    // 입력 받기
    static packet_buf_t pkt = {
        .vhdr = {0},
        .eth = {
            .dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
            .src_mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
            .ethertype = 0x0008},
        .payload = "Hello Kernel!"};

    // 전송하는 부분
    VIRTIO_QUEUE_SEL = 1;

    tx_queue.desc[0].addr = (uint64_t)&pkt;
    tx_queue.desc[0].len = sizeof(pkt);
    tx_queue.desc[0].flags = 0;
    tx_queue.desc[0].next = 0;

    tx_queue.avail->flags = 0;
    tx_queue.avail->ring[tx_queue.avail->idx % VIRTIO_QUEUE_SIZE] = 0;

    virtio_mb();
    tx_queue.avail->idx++;
    virtio_mb();

    VIRTIO_QUEUE_NOTIFY = 1;

    puts("Packet sent to TX queue, notified hardware!\n");

    puts("Waiting TX...\n");

    int timeout = 10000000;

    while (timeout--)
    {
        if (tx_queue.used->idx != last_tx_used_idx)
        {
            puts("TX SUCCESS\n");
            last_tx_used_idx++;
            return;
        }
    }

    puts("TX TIMEOUT\n");
}