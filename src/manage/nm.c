#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

/*
    네트워크 관련 함수가 있는 파일
*/

extern dcb_t nic_device;

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

struct virtio_queue_state
{
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
    unsigned char *storage;
};

/* Legacy split rings: used starts on the next 4 KiB boundary. */
#define VIRTIO_QUEUE_SIZE 256
#define VIRTIO_DESC_BYTES (16 * VIRTIO_QUEUE_SIZE)
#define VIRTIO_AVAIL_BYTES (4 + (2 * VIRTIO_QUEUE_SIZE))
#define VIRTIO_USED_OFFSET \
    (((VIRTIO_DESC_BYTES + VIRTIO_AVAIL_BYTES + 4095) / 4096) * 4096)
#define VIRTIO_USED_BYTES (4 + (8 * VIRTIO_QUEUE_SIZE))
#define VIRTIO_QUEUE_BYTES (VIRTIO_USED_OFFSET + VIRTIO_USED_BYTES)
#define VIRTIO_QUEUE_STORAGE \
    (((VIRTIO_QUEUE_BYTES + 4095) / 4096) * 4096)

static unsigned char rx_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static unsigned char tx_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static struct virtio_queue_state rx_queue;
static struct virtio_queue_state tx_queue;

void *get_ring_buffer_addr(void)
{
    return (void *)tx_queue.storage;
}

static unsigned char rx_packet_buffer[10 + 2048];

static uint16_t last_rx_used_idx = 0;
static uint16_t last_tx_used_idx = 0;

static inline void virtio_mb(void)
{
    __asm__ volatile("dmb ishst" ::: "memory");
}

static void setup_queue_state(int queue_index,
                              struct virtio_queue_state *queue)
{
    VIRTIO_QUEUE_SEL = queue_index;

    uint32_t max = VIRTIO_QUEUE_NUM_MAX;

    puts("Queue max supported: ");
    put_hex(max);
    puts("\n");

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

    puts("Queue PFN set: ");
    put_hex(VIRTIO_QUEUE_PFN);
    puts("\n");
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

void check_nic_completion(void)
{
    if (rx_queue.used->idx != last_rx_used_idx)
    {
        puts("NIC finished a receive job!\n");

        unsigned char *eth_frame = rx_packet_buffer + 10;

        // EtherType 확인 예시 (IPv4면 0x0800)
        uint16_t ethertype = (eth_frame[12] << 8) | eth_frame[13];

        last_rx_used_idx++;
    }

    if (tx_queue.used->idx != last_tx_used_idx)
    {
        puts("NIC finished a transmit job!\n");
        last_tx_used_idx++;
    }

    if (rx_queue.used->idx == last_rx_used_idx &&
        tx_queue.used->idx == last_tx_used_idx)
    {
        puts("Waiting for NIC response...\n");
    }
}

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

void debug_main(void)
{
    nic_device.init();

    setup_virtqueue(1); // TX만

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

// 나중에 따로 뺄 예정
void net_main(void)
{
    nic_device.init();

    setup_virtqueue(0); // RX만

    prepare_rx_buffer();

    puts("RX buffer addr = ");
    put_hex((uint64_t)rx_packet_buffer);
    puts("\n");

    puts("Descriptor addr = ");
    put_hex(rx_queue.desc[0].addr);
    puts("\n");

    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("RX Driver Ready\n");

    while (1)
    {
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