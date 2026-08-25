#include "manage/_nm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "tools/_asm.h"
#include "tools/_virtio.h"

/*
    네트워크 송신 및 관리 함수가 있는 파일
*/

extern dcb_t nic_device;

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

static unsigned char rx_packet_buffer[10 + 2048];

uint64_t nm_discap()
{
    /* 네트워크에서 받은 패킷을 Network Manager로 전달 */
    if (rx_queue.used->idx == last_rx_used_idx)
    {
        return 0;
    }

    puts("RX SUCCESS\n");

    struct virtq_used_elem *elem =
        &rx_queue.used->ring[last_rx_used_idx % VIRTIO_QUEUE_SIZE];

    packet_buf_t *pkt =
        (packet_buf_t *)rx_packet_buffer;

    int16_t id = -1;

    for (int i = 0; i < NETWORK_CACHE_SIZE; i++)
    {
        if (nm_connect.is_alloc[i] == 0)
        {
            nm_connect.is_alloc[i] = 1;
            id = i;
            break;
        }
    }

    if (id == -1)
    {
        return -1; // 더 이상 패킷을 받을 수 없음
    }

    uint8_t now_pkt = (uint8_t)id;

    nm_connect.nmqueue.push(&(nm_connect.nmqueue), now_pkt);

    /*
     * RX packet → Network Manager
     */
    memcpy(
        nm_connect.payload_buf[now_pkt],
        pkt->payload,
        1500);

    /*
     * 사용한 descriptor 재등록
     */
    rx_queue.avail->ring[rx_queue.avail->idx % VIRTIO_QUEUE_SIZE] = elem->id;

    virtio_mb();

    rx_queue.avail->idx++;

    virtio_mb();

    VIRTIO_QUEUE_NOTIFY = 0;

    last_rx_used_idx++;

    return 1;
}