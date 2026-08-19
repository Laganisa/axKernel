#include "manage/_nm.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    네트워크 수신 함수가 있는 파일
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

static inline uint64_t read_daif(void)
{
    uint64_t value;

    asm volatile(
        "mrs %0, daif"
        : "=r"(value));

    return value;
}

void net_RX_main(void)
{
    nic_device.init();

    setup_virtqueue(0);

    prepare_rx_buffer();

    dump("RX buffer addr", rx_packet_buffer);
    dump("Descriptor addr", rx_queue.desc[0].addr);

    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("RX Driver Ready\n");

    enable_irq();

    while (1)
    {
        if (rx_queue.used->idx == last_rx_used_idx)
            continue;

        puts("RX SUCCESS\n");

        struct virtq_used_elem *elem =
            &rx_queue.used->ring[last_rx_used_idx % VIRTIO_QUEUE_SIZE];

        packet_buf_t *pkt =
            (packet_buf_t *)rx_packet_buffer;

        dump("RX used idx", (uint64_t)rx_queue.used->idx);

        dump("VIRTIO INT STATUS", (uint64_t)VIRTIO_INTERRUPT_STATUS);

        // GIC Distributor Enable / Set-Pending 레지스터 매크로 반영
        dump("GIC ISPENDR", (uint64_t)GIC_DIST_REG(0x200)); // GICD_ISPENDR의 Distributor 내 오프셋 (0x200)

        dump("DAIF", (uint64_t)read_daif());

        dump("Descriptor", elem->id);
        dump("Length", elem->len);

        puts("\nDestination MAC : ");

        for (int i = 0; i < 6; i++)
        {
            put_hex2(pkt->eth.dst_mac[i]);
            puts(" ");
        }

        puts("\nSource MAC      : ");

        for (int i = 0; i < 6; i++)
        {
            put_hex2(pkt->eth.src_mac[i]);
            puts(" ");
        }

        puts("\n");

        dump("EtherType", pkt->eth.ethertype);

        uint32_t payload_len =
            elem->len -
            sizeof(virtio_net_hdr_t) -
            sizeof(eth_frame_t);

        puts("Payload : ");

        for (uint32_t i = 0; i < payload_len; i++)
        {
            putchar(pkt->payload[i]);
        }

        // for_dump("pkt->payload", pkt->payload, payload_len);

        puts("\n");

        /*
         * 다음 수신을 위해 Descriptor 재사용
         */

        rx_queue.avail->ring[rx_queue.avail->idx % VIRTIO_QUEUE_SIZE] = 0;

        virtio_mb();

        rx_queue.avail->idx++;

        virtio_mb();

        VIRTIO_QUEUE_NOTIFY = 0;

        last_rx_used_idx++;
    }
}