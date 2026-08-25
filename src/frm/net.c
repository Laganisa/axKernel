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

static unsigned char rx_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));

void nm_init()
{
    vq_init(g_virtio_net_base);

    vq_setup(g_virtio_net_base, 0, rx_queue_storage, &rx_queue);

    for (int i = 0; i < 6; i++)
    {
        nm_connect.dst_buf[0][i] = 0xFF;
    }

    nm_connect.is_dst[0] = 1;

    queue_init(&(nm_connect.nmqueue), nm_connect.nmbuf, NETWORK_CACHE_SIZE);
}

void net_RX_main(void)
{
    vq_init(g_virtio_net_base);

    vq_setup(g_virtio_net_base, 0, rx_queue_storage, &rx_queue);

    prepare_rx_buffer();

    dump("RX buffer addr", rx_packet_buffer);
    dump("Descriptor addr", rx_queue.desc[0].addr);

    GIC_DIST_CTRL = 1;

    // IRQ 79 → CPU 0
    GIC_DIST_REG8(0x84F) = 0x01;

    // IRQ 79 enable
    GIC_DIST_REG(0x108) |= (1U << 15);

    // CPU interface enable
    GIC_CPU_PMR = 0xFF;
    GIC_CPU_CTRL = 1;
    GIC_DIST_REG(0x108) |= (1U << 15);

    dump_("GIC ISENABLER2", GIC_DIST_REG(0x108));
    dump_("IRQ79 ENABLE",
          (GIC_DIST_REG(0x108) >> 15) & 1);

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