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

// 58B
typedef struct __attribute__((packed)) packet_buf_t
{
    virtio_net_hdr_t vhdr;
    eth_frame_t eth;
    char payload[32];
} packet_buf_t;

// 값 임시 매핑
#define NETWORK_CACHE_SIZE 10
#define DST_CACHE_SIZE 10

typedef struct NMv1_connect
{
    // 최근 목적지를 기억하는 캐시
    uint64_t dst_buf[DST_CACHE_SIZE];

    // ! 나중에 동적할당으로 바꾸기
    packet_buf_t payload_buf[NETWORK_CACHE_SIZE];
} NMv1_connect;

#define nm_connect ((NMv1_connect *)NM_ADDR_START)

void nm_cap(void);
void setup_virtqueue(int queue_index);
void check_nic_completion(void);

// 송신용도
void net_TX_main(void);

// 수신용도
void net_RX_main(void);

#endif