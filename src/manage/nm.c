#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "_vritio.h"

/*
    네트워크 관련 함수가 있는 파일
*/

extern dcb_t nic_device;

void net_send_test(void)
{
    static unsigned char packet[] = {
        // virtio_net_hdr (12 bytes)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

        // Ethernet frame
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x00,

        'H', 'e', 'l', 'l', 'o', ' ',
        'K', 'e', 'r', 'n', 'e', 'l', '!'};

    VIRTIO_QUEUE_SEL = 1;

    tx_queue.desc[0].addr = (uint64_t)packet;
    tx_queue.desc[0].len = sizeof(packet);
    tx_queue.desc[0].flags = 0;
    tx_queue.desc[0].next = 0;

    tx_queue.avail->flags = 0;
    tx_queue.avail->ring[tx_queue.avail->idx % VIRTIO_QUEUE_SIZE] = 0;

    virtio_mb();
    tx_queue.avail->idx++;
    virtio_mb();

    VIRTIO_QUEUE_NOTIFY = 1;

    puts("Packet sent to TX queue, notified hardware!\n");
}

void debug_main(void)
{
    nic_device.init();

    setup_virtqueue(1);

    // ? 무슨 뜻이지
    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("TX Driver Ready\n");

    net_send_test();

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
