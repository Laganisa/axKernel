#include "manage/_nm.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    네트워크 송신 및 관리 함수가 있는 파일
*/

extern dcb_t nic_device;

// 전송 로직 짜기
// 지금 단계는 패킷의 캡슐화가 가능한 상태
void nm_cap()
{
    static packet_buf_t pkt = {
        .vhdr = {0},
        .eth = {
            .dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
            .src_mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
            .ethertype = 0x0008},
        .payload = "Hello Kernel!"};

    /*
// 데이터 삽입
for (int i = 0; i < 6; i++)
{
    pkt.eth.dst_mac[i] = dst[i];
    pkt.eth.src_mac[i] = src[i];
}

// 이더넷 타입 넣기
pkt.eth.ethertype = type;

// 데이터 넣기
// ! 26이라고 하드코딩된 값 수정할 예정
for (int i = 0; i < 26; i++)
{
    pkt.payload[i] = data[i];
}
    */

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
}

void net_TX_main(void)
{
    nic_device.init();

    setup_virtqueue(1);

    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("TX Driver Ready\n");

    // 입력 받기

    nm_cap();

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