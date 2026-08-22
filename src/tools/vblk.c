#include "manage/_dm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    가상 큐 mmio를 이용한 디스크 코드
*/
extern uint64_t g_virtio_blk_base;

#define VIRTIO_BLK_STATUS (*(volatile uint32_t *)(g_virtio_blk_base + 0x070))

#define VIRTIO_BLK_HOST_FEATURES_SEL (*(volatile uint32_t *)(g_virtio_blk_base + 0x014))
#define VIRTIO_BLK_HOST_FEATURES (*(volatile uint32_t *)(g_virtio_blk_base + 0x010))

#define VIRTIO_BLK_GUEST_FEATURES_SEL (*(volatile uint32_t *)(g_virtio_blk_base + 0x024))
#define VIRTIO_BLK_GUEST_FEATURES (*(volatile uint32_t *)(g_virtio_blk_base + 0x020))

#define VIRTIO_BLK_QUEUE_SEL (*(volatile uint32_t *)(g_virtio_blk_base + 0x030))
#define VIRTIO_BLK_QUEUE_NUM_MAX (*(volatile uint32_t *)(g_virtio_blk_base + 0x034))
#define VIRTIO_BLK_QUEUE_NUM (*(volatile uint32_t *)(g_virtio_blk_base + 0x038))
#define VIRTIO_BLK_QUEUE_ALIGN (*(volatile uint32_t *)(g_virtio_blk_base + 0x03C))
#define VIRTIO_BLK_QUEUE_PFN (*(volatile uint32_t *)(g_virtio_blk_base + 0x040))

#define VIRTIO_BLK_GUEST_PAGE_SIZE (*(volatile uint32_t *)(g_virtio_blk_base + 0x028))

static unsigned char blk_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static struct virtio_queue_state blk_queue;
static void blk_setup_queue(void)
{
    VIRTIO_BLK_QUEUE_SEL = 0;

    uint32_t max = VIRTIO_BLK_QUEUE_NUM_MAX;

    dump("BLK queue max", max);

    if (max == 0)
    {
        puts("BLK queue unavailable\n");
        return;
    }

    if (max < VIRTIO_QUEUE_SIZE)
    {
        puts("BLK queue too small\n");
        return;
    }

    VIRTIO_BLK_QUEUE_NUM = VIRTIO_QUEUE_SIZE;
    VIRTIO_BLK_QUEUE_ALIGN = 4096;

    blk_queue.desc =
        (struct virtq_desc *)blk_queue_storage;

    blk_queue.avail =
        (struct virtq_avail *)(blk_queue_storage + VIRTIO_DESC_BYTES);

    blk_queue.used =
        (struct virtq_used *)(blk_queue_storage + VIRTIO_USED_OFFSET);

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        blk_queue_storage[i] = 0;
    }

    VIRTIO_BLK_QUEUE_PFN =
        ((uint64_t)blk_queue_storage) >> 12;

    dump("BLK queue PFN",
         VIRTIO_BLK_QUEUE_PFN);

    puts("BLK queue initialized\n");
}

void blk_init(void)
{
    if (g_virtio_blk_base == 0)
    {
        puts("BLK MMIO base not found\n");
        return;
    }

    VIRTIO_BLK_STATUS = 0;

    VIRTIO_BLK_STATUS |= VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_BLK_STATUS |= VIRTIO_STATUS_DRIVER;

    VIRTIO_BLK_HOST_FEATURES_SEL = 0;

    uint32_t host_features =
        VIRTIO_BLK_HOST_FEATURES;

    dump("BLK host features", host_features);

    VIRTIO_BLK_GUEST_FEATURES_SEL = 0;
    VIRTIO_BLK_GUEST_FEATURES = 0;

    VIRTIO_BLK_GUEST_PAGE_SIZE = 4096;

    VIRTIO_BLK_STATUS |= VIRTIO_STATUS_FEATURES_OK;

    if ((VIRTIO_BLK_STATUS &
         VIRTIO_STATUS_FEATURES_OK) == 0)
    {
        puts("BLK FEATURES_OK rejected\n");
        return;
    }

    dump("BLK status", VIRTIO_BLK_STATUS);
    blk_setup_queue();
    VIRTIO_BLK_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    dump("BLK final status", VIRTIO_BLK_STATUS);

    puts("BLK basic initialization successful\n");
}