#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03C
#define VIRTIO_MMIO_QUEUE_PFN 0x040
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_DRIVER_LOW 0x090
#define VIRTIO_MMIO_QUEUE_DRIVER_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_DEVICE_LOW 0x0A0
#define VIRTIO_MMIO_QUEUE_DEVICE_HIGH 0x0A4
#define VIRTIO_MMIO_CONFIG 0x100

#define VIRTIO_STATUS_FAILED 0x80

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
    vq->size = vq_size;

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

    VIRTIO_TOT_REG(v_base_addr, 0x020) = 0;
    VIRTIO_TOT_REG(v_base_addr, 0x028) = 4096;
}

uint32_t virtio_mmio_version(uint64_t v_base_addr)
{
    return VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_VERSION);
}

void virtio_mmio_notify_queue(uint64_t v_base_addr, uint16_t q_index)
{
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_NOTIFY) = q_index;
}

int vq_setup_v1(
    uint64_t v_base_addr,
    uint16_t q_index,
    uint8_t *storage,
    struct virtio_queue_state *vq)
{
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_SEL) = q_index;

    uint32_t queue_max =
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (queue_max == 0)
    {
        dump("VirtIO queue unavailable", q_index);
        return -1;
    }

    uint32_t queue_size = VIRTIO_QUEUE_SIZE;
    while (queue_size > queue_max)
    {
        queue_size >>= 1;
    }

    if (queue_size == 0)
    {
        dump("VirtIO queue size invalid", queue_max);
        return -1;
    }

    vq->storage = storage;
    vq->desc = (struct virtq_desc *)storage;
    vq->avail = (struct virtq_avail *)(storage +
                                       queue_size * sizeof(struct virtq_desc));

    uint32_t used_offset = ALIGN_4K(
        queue_size * sizeof(struct virtq_desc) +
        sizeof(uint16_t) * 2 +
        queue_size * sizeof(uint16_t));
    vq->used = (struct virtq_used *)(storage + used_offset);
    vq->size = queue_size;

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        storage[i] = 0;
    }

    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_GUEST_PAGE_SIZE) = 4096;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_ALIGN) = 4096;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_NUM) = queue_size;

    virtio_mb();
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_PFN) =
        (uint32_t)((uintptr_t)storage >> 12);

    return 0;
}

void vq_start_v1(uint64_t v_base_addr)
{
    virtio_mb();
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_DRIVER_OK;
}

int vq_init_v2(uint64_t v_base_addr)
{
    uint32_t version = VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_VERSION);
    if (version != 2)
    {
        dump("VirtIO MMIO version", version);
        puts("VirtIO MMIO version 2 required\n");
        return -1;
    }

    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) = 0;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_DRIVER;

    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_DEVICE_FEATURES_SEL) = 1;
    uint32_t features_hi =
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_DEVICE_FEATURES);

    if ((features_hi & 1U) == 0)
    {
        puts("VirtIO VERSION_1 feature not found\n");
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_FAILED;
        return -1;
    }

    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_DRIVER_FEATURES_SEL) = 0;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_DRIVER_FEATURES) = 0;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_DRIVER_FEATURES_SEL) = 1;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_DRIVER_FEATURES) = 1;

    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_FEATURES_OK;
    if ((VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) &
         VIRTIO_STATUS_FEATURES_OK) == 0)
    {
        puts("VirtIO features rejected\n");
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_FAILED;
        return -1;
    }

    return 0;
}

int vq_setup_v2(
    uint64_t v_base_addr,
    uint16_t q_index,
    uint8_t *storage,
    struct virtio_queue_state *vq)
{
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_SEL) = q_index;

    uint32_t queue_max =
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (queue_max == 0)
    {
        dump("VirtIO queue unavailable", q_index);
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_FAILED;
        return -1;
    }

    uint32_t queue_size = VIRTIO_QUEUE_SIZE;
    while (queue_size > queue_max)
    {
        queue_size >>= 1;
    }

    if (queue_size == 0)
    {
        dump("VirtIO queue size invalid", queue_max);
        VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_FAILED;
        return -1;
    }

    vq->storage = storage;
    vq->desc = (struct virtq_desc *)storage;
    vq->avail = (struct virtq_avail *)(storage + VIRTIO_DESC_BYTES);
    vq->used = (struct virtq_used *)(storage + VIRTIO_USED_OFFSET);
    vq->size = queue_size;

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        storage[i] = 0;
    }

    uint64_t desc_addr = (uint64_t)(uintptr_t)vq->desc;
    uint64_t driver_addr = (uint64_t)(uintptr_t)vq->avail;
    uint64_t device_addr = (uint64_t)(uintptr_t)vq->used;

    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_NUM) = queue_size;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint32_t)desc_addr;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint32_t)(desc_addr >> 32);
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_DRIVER_LOW) = (uint32_t)driver_addr;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_DRIVER_HIGH) = (uint32_t)(driver_addr >> 32);
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_DEVICE_LOW) = (uint32_t)device_addr;
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_DEVICE_HIGH) = (uint32_t)(device_addr >> 32);

    virtio_mb();
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_QUEUE_READY) = 1;

    return 0;
}

uint32_t virtio_mmio_read_config32(uint64_t v_base_addr, uint32_t offset)
{
    return VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_CONFIG + offset);
}

void vq_start_v2(uint64_t v_base_addr)
{
    virtio_mb();
    VIRTIO_TOT_REG(v_base_addr, VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_DRIVER_OK;
}