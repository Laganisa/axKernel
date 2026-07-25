#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "_vritio.h"

/*
    네트워크 관련 함수가 있는 파일
*/

extern dcb_t nic_device;

static unsigned char rx_packet_buffer[10 + 2048];

void prepare_rx_buffer(void)
{
    VIRTIO_QUEUE_SEL = 0;

    rx_queue.desc[0].addr = (uint64_t)rx_packet_buffer;
    rx_queue.desc[0].len = sizeof(rx_packet_buffer);
    rx_queue.desc[0].flags = VIRTQ_DESC_F_WRITE;
    rx_queue.desc[0].next = 0;

    rx_queue.avail->flags = 0;
    rx_queue.avail->ring[rx_queue.avail->idx % VIRTIO_QUEUE_SIZE] = 0;

    virtio_mb();
    rx_queue.avail->idx++;
    virtio_mb();

    VIRTIO_QUEUE_NOTIFY = 0;
}

void net_main(void)
{
    nic_device.init();

    setup_virtqueue(0);

    prepare_rx_buffer();

    dump("RX buffer addr = ", rx_packet_buffer);
    dump("Descriptor addr = ", rx_queue.desc[0].addr);

    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("RX Driver Ready\n");

    while (1)
    {
        // 단순 출력부
        if (rx_queue.used->idx != last_rx_used_idx)
        {
            puts("RX SUCCESS\n");

            struct virtq_used_elem *elem =
                &rx_queue.used->ring[last_rx_used_idx % VIRTIO_QUEUE_SIZE];

            puts("Descriptor : ");
            put_hex(elem->id);
            puts("\n");

            puts("Length : ");
            put_hex(elem->len);
            puts("\n");

            unsigned char *frame = rx_packet_buffer + 12;

            puts("RX BUFFER:\n");

            for (uint32_t i = 0; i < elem->len; i++)
            {
                put_hex2(rx_packet_buffer[i]);
                puts(" ");

                if ((i & 15) == 15)
                {
                    puts("\n");
                }
            }

            puts("\nPayload:\n");

            unsigned char *payload = frame + 14;
            uint32_t payload_len = elem->len - 12 - 14;

            puts("\nPayload:\n");

            for (uint32_t i = 0; i < payload_len; i++)
            {
                put_hex2(payload[i]);
                puts(" ");
            }
            puts("\n");

            puts("\nPayload ASCII:\n");

            for (uint32_t i = 0; i < payload_len; i++)
            {
                putchar((char)payload[i]);
            }

            puts("\n");

            last_rx_used_idx++;
        }
    }
}