#ifndef __KERNEL_NM_H__
#define __KERNEL_NM_H__

#include "_defs.h"
#include "_types.h"
#include "_macro.h"
#include "manage/_dm.h"

typedef struct __attribute__((packed)) virtio_net_hdr
{
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;

} virtio_net_hdr_t;

//
typedef struct __attribute__((packed)) eth_frame
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} eth_frame_t;

typedef struct __attribute__((packed))
{
    virtio_net_hdr_t vhdr;

    eth_frame_t eth;

    uint8_t payload[1500];

} packet_buf_t;

// 값 임시 매핑
#define NETWORK_CACHE_SIZE 10
#define DST_CACHE_SIZE 10

typedef struct NMv1_connect
{
    // 최근 목적지를 기억하는 캐시
    uint8_t is_dst[DST_CACHE_SIZE];
    uint8_t dst_buf[DST_CACHE_SIZE][6];

    // ! 나중에 동적할당으로 바꾸기
    /*
        나중에 큐 자료 구조를 이용하기
    */
    uint8_t head;
    uint8_t tail;
    uint8_t num;
    uint8_t queue_buf[NETWORK_CACHE_SIZE];
    packet_buf_t payload_buf[NETWORK_CACHE_SIZE];
} NMv1_connect;

#define nm_connect ((NMv1_connect *)NM_ADDR_START)

void nm_cap(uint8_t *dst, const void *data, uint16_t len, uint16_t type);
uint64_t nm_discap();

void nm_init();

uint8_t nm_queue(NMv1_connect *queue, uint8_t cmd, uint8_t val); // 송신용도

// 송신 용도
void net_TX_main(void);

// 수신용도
void net_RX_main(void);

#endif