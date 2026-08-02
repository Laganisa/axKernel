#include "manage/_nm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "tools/_asm.h"
#include "tools/_virtio.h"

/*
    네트워크 송신 및 관리 함수가 있는 파일
*/

extern dcb_t nic_device;

void nm_init()
{
    nic_device.init();

    setup_virtqueue(1);

    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("TX Driver Ready\n");

    for (int i = 0; i < 6; i++)
    {
        nm_connect->dst_buf[0][i] = 0xFF;
    }

    nm_connect->is_dst[0] = 1;
}

/*
    전송하는 함수
*/
void nm_cap(uint8_t *dst, const void *data, uint16_t len, uint16_t type)
{
    enter("nm_cap");

    static packet_buf_t pkt;

    // VirtIO Header 초기화
    memset(&pkt.vhdr, 0, sizeof(pkt.vhdr));

    // Destination MAC
    memcpy(pkt.eth.dst_mac, dst, 6);

    // Source MAC
    static const uint8_t src_mac[6] =
        {
            0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

    memcpy(pkt.eth.src_mac, src_mac, 6);

    // EtherType
    pkt.eth.ethertype = type;

    // Payload
    memcpy(pkt.payload, data, len);

    uint32_t packet_size =
        sizeof(virtio_net_hdr_t) +
        sizeof(eth_frame_t) +
        len;

    // TX Queue
    VIRTIO_QUEUE_SEL = 1;

    tx_queue.desc[0].addr = (uint64_t)&pkt;
    tx_queue.desc[0].len = packet_size;
    tx_queue.desc[0].flags = 0;
    tx_queue.desc[0].next = 0;

    tx_queue.avail->flags = 0;
    tx_queue.avail->ring[tx_queue.avail->idx % VIRTIO_QUEUE_SIZE] = 0;

    virtio_mb();

    tx_queue.avail->idx++;

    virtio_mb();

    VIRTIO_QUEUE_NOTIFY = 1;

    puts("TX COMPLETE\n");
}

uint64_t nm_discap()
{
    /*네트워크에서 받는 함수*/
}

/*
    0 : 집어 넣기
    1 : 빼기
*/
uint8_t nm_queue(NMv1_connect *queue, uint8_t cmd, uint8_t val)
{
    if (cmd == 0)
    {
        queue->queue_buf[queue->head] = val;
        queue->head = (queue->head + 1) & 255;
        queue->num++;
        return 0;
    }

    if (queue->num == 0)
    {
        return 0;
    }
    uint8_t ret = queue->queue_buf[queue->tail];
    queue->tail = (queue->tail + 1) & 255;
    queue->num--;
    return ret;
}

void net_TX_main(void)
{
    nic_device.init();

    setup_virtqueue(1);

    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("TX Driver Ready\n");

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