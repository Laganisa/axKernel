#include "manage/_nm.h"

/*
    네트워크 관련 함수가 있는 파일
*/

extern dcb_t nic_device;

static unsigned char virtio_ring_buffer[4096] __attribute__((aligned(4096)));

void *get_ring_buffer_addr(void)
{
    return (void *)virtio_ring_buffer;
}

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
    uint16_t ring[128];
    uint16_t used_event;
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
    struct virtq_used_elem ring[128];
};

struct virtio_queue_state
{
    struct virtq_desc desc[128] __attribute__((aligned(4096)));
    struct virtq_avail avail __attribute__((aligned(4096)));
    struct virtq_used used __attribute__((aligned(4096)));
};

static struct virtio_queue_state rx_queue;
static struct virtio_queue_state tx_queue;

static unsigned char rx_packet_buffer[2048];

static uint16_t last_rx_used_idx = 0;
static uint16_t last_tx_used_idx = 0;

static inline void virtio_mb(void)
{
    __asm__ volatile("" ::: "memory");
}

static void setup_queue_state(int queue_index, struct virtio_queue_state *queue)
{
    VIRTIO_QUEUE_SEL = queue_index;

    VIRTIO_QUEUE_NUM = 128;

    VIRTIO_QUEUE_DESC_LOW = (uint32_t)((uint64_t)queue->desc & 0xFFFFFFFFU);
    VIRTIO_QUEUE_DESC_HIGH = (uint32_t)(((uint64_t)queue->desc >> 32) & 0xFFFFFFFFU);
    VIRTIO_QUEUE_AVAIL_LOW = (uint32_t)((uint64_t)&queue->avail & 0xFFFFFFFFU);
    VIRTIO_QUEUE_AVAIL_HIGH = (uint32_t)(((uint64_t)&queue->avail >> 32) & 0xFFFFFFFFU);
    VIRTIO_QUEUE_USED_LOW = (uint32_t)((uint64_t)&queue->used & 0xFFFFFFFFU);
    VIRTIO_QUEUE_USED_HIGH = (uint32_t)(((uint64_t)&queue->used >> 32) & 0xFFFFFFFFU);

    VIRTIO_QUEUE_READY = 1;
}

void setup_virtqueue(int queue_index)
{
    if (queue_index == 0)
    {
        setup_queue_state(queue_index, &rx_queue);
    }
    else
    {
        setup_queue_state(queue_index, &tx_queue);
    }

    puts("Queue setup done with Desc Table!\n");
}

void check_nic_completion(void)
{
    if (rx_queue.used.idx != last_rx_used_idx)
    {
        puts("NIC finished a receive job!\n");
        last_rx_used_idx++;
    }

    if (tx_queue.used.idx != last_tx_used_idx)
    {
        puts("NIC finished a transmit job!\n");
        last_tx_used_idx++;
    }

    if (rx_queue.used.idx == last_rx_used_idx && tx_queue.used.idx == last_tx_used_idx)
    {
        puts("Waiting for NIC response...\n");
    }
}

void net_send_test(void)
{
    static unsigned char packet[12 + 64] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // virtio_net_hdr (12 bytes)
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest MAC (Broadcast)
        0x02, 0x00, 0x00, 0x00, 0x00, 0x01, // Src MAC
        0x08, 0x00,                         // EtherType (IPv4)
        'H', 'e', 'l', 'l', 'o', ' ', 'K', 'e', 'r', 'n', 'e', 'l', '!'};

    VIRTIO_QUEUE_SEL = 1;

    // 💡 디스크립터 세팅 명확화
    tx_queue.desc[0].addr = (uint64_t)packet;
    tx_queue.desc[0].len = 12 + 64; // 명시적으로 헤더(12) + 페이로드(64) 지정
    tx_queue.desc[0].flags = 0;     // 체이닝 없음 (단일 디스크립터)
    tx_queue.desc[0].next = 0;

    // Available 링 등록
    tx_queue.avail.flags = 0;
    tx_queue.avail.ring[tx_queue.avail.idx % 128] = 0;

    virtio_mb(); // 메모리 배리어로 순서 보장
    tx_queue.avail.idx++;
    virtio_mb();

    // 하드웨어에 1번 큐(TX)에 일감이 생겼다고 알림!
    VIRTIO_QUEUE_NOTIFY = 1;

    puts("Packet sent to TX queue, notified hardware!\n");
}

void prepare_rx_buffer(void)
{
    VIRTIO_QUEUE_SEL = 0;

    rx_queue.desc[0].addr = (uint64_t)rx_packet_buffer;
    rx_queue.desc[0].len = sizeof(rx_packet_buffer);
    rx_queue.desc[0].flags = VIRTQ_DESC_F_WRITE; // 장치가 쓸 수 있도록 쓰기 권한 부여
    rx_queue.desc[0].next = 0;

    rx_queue.avail.flags = 0;

    // 💡 수신 큐 쪽도 동일하게 모듈러 연산 적용
    rx_queue.avail.ring[rx_queue.avail.idx % 128] = 0;
    virtio_mb();
    rx_queue.avail.idx++;
    virtio_mb();

    VIRTIO_QUEUE_NOTIFY = 0;
}

void debug_main(void)
{
    // 1. 장치 초기화 및 피처 협상
    nic_device.init();

    // 2. 큐 주소 세팅 (RX, TX)
    setup_virtqueue(0);
    setup_virtqueue(1);

    // 3. 수신 버퍼 미리 걸어두기
    prepare_rx_buffer();

    // 4. 모든 큐 세팅 완료 후 DRIVER_OK 점화
    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;
    puts("NIC is fully ready and DRIVER_OK set!\n");

    // 5. 패킷 송신 테스트!
    net_send_test();

    // 6. 💡 하드웨어가 TX 작업을 끝내고 Used Ring을 갱신할 때까지 적극적으로 대기
    puts("Waiting for TX completion...\n");
    int timeout = 10000000;
    while (timeout--)
    {
        if (tx_queue.used.idx != last_tx_used_idx)
        {
            puts("SUCCESS: TX packet successfully processed by hardware!\n");
            last_tx_used_idx++;
            break;
        }
    }

    if (timeout <= 0)
    {
        puts("TIMEOUT: Hardware did not respond to TX queue.\n");
    }
}