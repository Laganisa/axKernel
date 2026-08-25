#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    가상 큐 관련 파일
*/

void vq_setup(
    uint64_t v_base_addr,
    int q_index,
    uint8_t *storage,
    struct virtio_queue_state *vq)
{
    VIRTIO_TOT_REG(v_base_addr, 0x030) = q_index;

    uint32_t max = VIRTIO_TOT_REG(v_base_addr, 0x034);

    uint32_t vq_size;

    if (max < VIRTIO_QUEUE_SIZE)
    {
        vq_size = max;
    }
    else
    {
        vq_size = VIRTIO_QUEUE_SIZE;
    }

    VIRTIO_TOT_REG(v_base_addr, 0x028) = 4096;
    VIRTIO_TOT_REG(v_base_addr, 0x03C) = 4096;
    VIRTIO_TOT_REG(v_base_addr, 0x038) = vq_size;

    vq->storage = storage;
    vq->desc = (struct virtq_desc *)vq->storage;
    vq->avail = (struct virtq_avail *)(vq->storage + VIRTIO_DESC_BYTES);
    vq->used = (struct virtq_used *)(vq->storage + VIRTIO_USED_OFFSET);

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        vq->storage[i] = 0;
    }

    VIRTIO_TOT_REG(v_base_addr, 0x040) = ((uint64_t)vq->storage) >> 12;

    // 성공 플래그
    VIRTIO_TOT_REG(v_base_addr, 0x070) |= VIRTIO_STATUS_DRIVER_OK;

    puts("Queue init successfully\n");
}

void vq_init(uint64_t v_base_addr)
{
    VIRTIO_TOT_REG(v_base_addr, 0x070) = 0;

    VIRTIO_TOT_REG(v_base_addr, 0x070) = VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_TOT_REG(v_base_addr, 0x070) |= VIRTIO_STATUS_DRIVER;

    uint32_t host_features = VIRTIO_TOT_REG(v_base_addr, 0x010);

    VIRTIO_TOT_REG(v_base_addr, 0x020) = 0;
    VIRTIO_TOT_REG(v_base_addr, 0x024) = 0;
    VIRTIO_TOT_REG(v_base_addr, 0x028) = 4096;

    VIRTIO_TOT_REG(v_base_addr, 0x070) |= VIRTIO_STATUS_FEATURES_OK;

    puts("host features successfully\n");

    if ((VIRTIO_TOT_REG(v_base_addr, 0x070) &
         VIRTIO_STATUS_FEATURES_OK) == 0)
    {
        puts("FEATURES_OK rejected\n");
        return;
    }
}