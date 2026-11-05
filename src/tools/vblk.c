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

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

#define VIRTIO_BLK_S_OK 0

#define VRING_DESC_F_NEXT 1
#define VRING_DESC_F_WRITE 2

#define VIRTIO_BLK_QUEUE_NOTIFY \
    (*(volatile uint32_t *)(g_virtio_blk_base + 0x050))

struct virtio_blk_req
{
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

enum blk_request_state
{
    BLK_REQ_IDLE,
    BLK_REQ_PENDING,
    BLK_REQ_COMPLETE,
    BLK_REQ_ERROR
};

static volatile enum blk_request_state blk_req_state =
    BLK_REQ_IDLE;

static struct virtio_blk_req blk_req
    __attribute__((aligned(16)));

static uint8_t blk_status
    __attribute__((aligned(1)));

#define VIRTIO_BLK_INTERRUPT_STATUS \
    (*(volatile uint32_t *)(g_virtio_blk_base + 0x060))

static volatile uint16_t blk_last_used_idx;
static uint64_t blk_sector_count;
static uint8_t blk_is_ready;

static int blk_submit(
    uint8_t cmd,
    uint32_t sector,
    void *buffer)
{
    if (!blk_is_ready || buffer == NULL ||
        blk_req_state == BLK_REQ_PENDING ||
        (cmd != VIRTIO_BLK_T_IN && cmd != VIRTIO_BLK_T_OUT) ||
        (uint64_t)sector >= blk_sector_count)
    {
        return FALSE;
    }

    struct virtq_desc *desc = blk_queue.desc;

    blk_req.type = cmd;
    blk_req.reserved = 0;
    blk_req.sector = sector;

    blk_status = 0xFF;

    desc[0].addr = (uint64_t)&blk_req;
    desc[0].len = sizeof(struct virtio_blk_req);
    desc[0].flags = VRING_DESC_F_NEXT;
    desc[0].next = 1;
    desc[1].addr = (uint64_t)buffer;
    desc[1].len = 512;
    desc[1].flags = VRING_DESC_F_NEXT;

    if (cmd == VIRTIO_BLK_T_IN)
    {
        desc[1].flags |= VRING_DESC_F_WRITE;
    }

    desc[1].next = 2;
    desc[2].addr = (uint64_t)&blk_status;
    desc[2].len = sizeof(blk_status);
    desc[2].flags = VRING_DESC_F_WRITE;

    struct virtq_avail *avail = blk_queue.avail;
    uint16_t index = avail->idx;
    avail->ring[index % VIRTIO_QUEUE_SIZE] = 0;

    virtio_mb();
    avail->idx = index + 1;
    blk_req_state = BLK_REQ_PENDING;
    virtio_mb();

    VIRTIO_BLK_QUEUE_NOTIFY = 0;

    while (blk_req_state == BLK_REQ_PENDING)
    {
        uint16_t used_idx = blk_queue.used->idx;
        if (used_idx != blk_last_used_idx)
        {
            virtio_mb();
            blk_last_used_idx = used_idx;
            blk_req_state =
                blk_status == VIRTIO_BLK_S_OK
                    ? BLK_REQ_COMPLETE
                    : BLK_REQ_ERROR;
        }
    }

    uint8_t success = blk_req_state == BLK_REQ_COMPLETE;
    blk_req_state = BLK_REQ_IDLE;
    return success;
}

void blk_init(void)
{
    if (blk_is_ready)
    {
        return;
    }

    if (g_virtio_blk_base == 0)
    {
        puts("BLK MMIO base not found\n");
        return;
    }

    vq_init(g_virtio_blk_base);
    vq_setup(g_virtio_blk_base, 0, blk_queue_storage, &blk_queue);
    if (blk_queue.size < 3)
    {
        puts("BLK queue is too small\n");
        return;
    }

    blk_last_used_idx = blk_queue.used->idx;
    blk_sector_count =
        (uint64_t)virtio_mmio_read_config32(g_virtio_blk_base, 0) |
        ((uint64_t)virtio_mmio_read_config32(g_virtio_blk_base, 4) << 32);
    blk_is_ready = blk_sector_count != 0;
    if (!blk_is_ready)
    {
        puts("BLK capacity unavailable\n");
    }
}

uint8_t virtio_blk_is_ready(void)
{
    return blk_is_ready;
}

uint64_t virtio_blk_sector_count(void)
{
    return blk_sector_count;
}

uint8_t virtio_blk_read_sector(uint32_t sector, void *buffer)
{
    return (uint8_t)blk_submit(VIRTIO_BLK_T_IN, sector, buffer);
}

uint8_t virtio_blk_write_sector(uint32_t sector, const void *buffer)
{
    return (uint8_t)blk_submit(VIRTIO_BLK_T_OUT, sector, (void *)buffer);
}

#define VIRTIO_BLK_INTERRUPT_ACK \
    (*(volatile uint32_t *)(g_virtio_blk_base + 0x064))

void virtio_blk_irq_handle(void)
{
    uint32_t status = VIRTIO_BLK_INTERRUPT_STATUS;

    if (status == 0)
    {
        return;
    }

    VIRTIO_BLK_INTERRUPT_ACK = status;

    struct virtq_used *used = blk_queue.used;

    if (used->idx == blk_last_used_idx)
    {
        return;
    }

    blk_last_used_idx = used->idx;

    if (blk_status == VIRTIO_BLK_S_OK)
    {
        blk_req_state = BLK_REQ_COMPLETE;
    }
    else
    {
        blk_req_state = BLK_REQ_ERROR;
    }
}
