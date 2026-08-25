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
#define BLK_TEST_SECTOR 100

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

static int blk_write_test(void);

static struct virtio_blk_req blk_req
    __attribute__((aligned(16)));

static uint8_t blk_data[512]
    __attribute__((aligned(16)));

static uint8_t blk_status
    __attribute__((aligned(1)));

static void blk_prepare_test_data(void)
{
    memset(blk_data, 0, sizeof(blk_data));

    const char *msg = "HELLO FROM axOS";

    for (uint32_t i = 0; msg[i] != '\0'; ++i)
    {
        blk_data[i] = (uint8_t)msg[i];
    }
}

#define VIRTIO_BLK_INTERRUPT_STATUS \
    (*(volatile uint32_t *)(g_virtio_blk_base + 0x060))

static uint8_t blk_read_data[512]
    __attribute__((aligned(16)));

/*
    cmd == 0 쓰기
    cmd == 1 읽기
*/
static int blk_submit(uint8_t cmd)
{
    struct virtq_desc *desc = blk_queue.desc;

    blk_req.type = VIRTIO_BLK_T_IN;
    blk_req.reserved = 0;
    blk_req.sector = BLK_TEST_SECTOR;

    blk_status = 0xFF;

    desc[0].addr = (uint64_t)&blk_req;
    desc[0].len = sizeof(struct virtio_blk_req);
    desc[0].flags = VRING_DESC_F_NEXT;
    desc[0].next = 1;
    desc[1].addr = (uint64_t)blk_read_data;
    desc[1].len = sizeof(blk_read_data);
    desc[1].flags = VRING_DESC_F_NEXT;

    if (cmd == 1)
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
}

void blk_init(void)
{
    // 주소 확인
    if (g_virtio_blk_base == 0)
    {
        puts("BLK MMIO base not found\n");
        return;
    }

    // 피쳐
    vq_init(g_virtio_blk_base);

    // 큐 설정
    vq_setup(g_virtio_blk_base, 0, blk_queue_storage, &blk_queue);

    const char *msg = "HELLO FROM axOS";

    for (uint32_t i = 0; msg[i] != '\0'; ++i)
    {
        blk_data[i] = (uint8_t)msg[i];
    }

    blk_submit(0);

    // ! 이거 바꿀거
    while (blk_req_state == BLK_REQ_PENDING)
    {
        asm volatile("wfi");
    }

    memset(blk_read_data, 0, sizeof(blk_read_data));

    blk_submit(1);

    while (blk_req_state == BLK_REQ_PENDING)
    {
        asm volatile("wfi");
    }

    if (blk_req_state == BLK_REQ_COMPLETE)
    {
        puts("BLK READ COMPLETE\n");

        puts((char *)blk_read_data);
    }

    log("1");

    log("READ BYTE");

    for (int i = 0; i < 512; i++)
    {
        if (blk_read_data[i] != 0)
        {
            put_hex2(blk_read_data[i]);
        }
    }
    puts("\n\n");
    log("Program end");
}

#define VIRTIO_BLK_INTERRUPT_STATUS \
    (*(volatile uint32_t *)(g_virtio_blk_base + 0x060))

#define VIRTIO_BLK_INTERRUPT_ACK \
    (*(volatile uint32_t *)(g_virtio_blk_base + 0x064))

static uint16_t blk_last_used_idx = 0;

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
        puts("BLK REQUEST COMPLETE\n");
    }
    else
    {
        blk_req_state = BLK_REQ_ERROR;
        puts("BLK WRITE ERROR\n");
    }
}