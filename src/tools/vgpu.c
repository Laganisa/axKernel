#include "manage/_nm.h"
#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
    가상 큐 mmio를 이용한 gpu 코드
*/

#define GPU_RESOURCE_ID 1
#define GPU_SCANOUT_ID 0

#define GPU_DEFAULT_WIDTH 640
#define GPU_DEFAULT_HEIGHT 360

#define GPU_CMD_SLOT_SIZE 4096
#define GPU_CMD_SLOT_COUNT 8

static unsigned char gpu_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));

static virtio_queue_state gpu_queue;

static uint16_t gpu_used_idx = 0;
static uint32_t gpu_queue_size = 0;

/* 현재 디스플레이 정보 */
uint32_t gpu_display_width = GPU_DEFAULT_WIDTH;
uint32_t gpu_display_height = GPU_DEFAULT_HEIGHT;

/* Framebuffer (고정 주소을 직접 가리키는 포인터) */
volatile uint32_t *gpu_framebuffer = (volatile uint32_t *)0x41000000;

/* GPU Command/Response Slots */
static unsigned char
    gpu_cmd_slot[GPU_CMD_SLOT_COUNT][GPU_CMD_SLOT_SIZE]
    __attribute__((aligned(4096)));

static unsigned char
    gpu_resp_slot[GPU_CMD_SLOT_COUNT][GPU_CMD_SLOT_SIZE]
    __attribute__((aligned(4096)));

static int gpu_wait_used(void)
{
    int timeout = 10000000;

    while (1)
    {
        virtio_mb();

        if (gpu_queue.used->idx != gpu_used_idx)
            break;

        if (--timeout == 0)
        {
            puts("GPU queue timed out\n");
            return -1;
        }
    }

    uint32_t irq_status = VIRTIO_GPU_INTERRUPT_STATUS;

    if (irq_status != 0)
    {
        VIRTIO_GPU_INTERRUPT_ACK = irq_status;
    }

    uint16_t new_used_idx = gpu_queue.used->idx;

    gpu_used_idx = new_used_idx;

    return 0;
}

static int gpu_submit_control(
    void *cmd,
    uint32_t cmd_size,
    void *resp,
    uint32_t resp_size)
{

    VIRTIO_GPU_QUEUE_SEL = 0;

    if (cmd_size > GPU_CMD_SLOT_SIZE)
        return -1;

    if (resp_size > GPU_CMD_SLOT_SIZE)
        return -1;

    uint16_t avail_idx =
        gpu_queue.avail->idx;

    uint16_t ring_index =
        avail_idx % gpu_queue_size;

    uint16_t head =
        (ring_index * 2) % gpu_queue_size;

    uint16_t next_desc =
        (head + 1) % gpu_queue_size;

    uint16_t slot =
        ring_index % GPU_CMD_SLOT_COUNT;

    uint64_t cmd_phys =
        (uint64_t)(uintptr_t)
            gpu_cmd_slot[slot];

    uint64_t resp_phys =
        (uint64_t)(uintptr_t)
            gpu_resp_slot[slot];

    /*
     * Command
     */
    for (uint32_t i = 0; i < cmd_size; ++i)
    {
        gpu_cmd_slot[slot][i] =
            ((unsigned char *)cmd)[i];
    }

    /*
     * Response buffer 초기화
     */
    for (uint32_t i = 0;
         i < resp_size;
         ++i)
    {
        gpu_resp_slot[slot][i] = 0;
    }

    /*
     * Descriptor 0: command
     */
    gpu_queue.desc[head].addr =
        cmd_phys;

    gpu_queue.desc[head].len =
        cmd_size;

    gpu_queue.desc[head].flags =
        VIRTQ_DESC_F_NEXT;

    gpu_queue.desc[head].next =
        next_desc;

    /*
     * Descriptor 1: response
     */
    gpu_queue.desc[next_desc].addr =
        resp_phys;

    gpu_queue.desc[next_desc].len =
        resp_size;

    gpu_queue.desc[next_desc].flags =
        VIRTQ_DESC_F_WRITE;

    gpu_queue.desc[next_desc].next = 0;

    /*
     * Avail
     */
    gpu_queue.avail->ring[ring_index] =
        head;

    virtio_mb();

    gpu_queue.avail->idx =
        avail_idx + 1;

    virtio_mb();

    VIRTIO_GPU_QUEUE_NOTIFY = 0;

    /*
     * Wait
     */
    if (gpu_wait_used() < 0)
    {
        return -1;
    }

    /*
     * Used 확인
     */
    uint16_t current_used_idx =
        gpu_queue.used->idx;

    uint16_t used_ring_index =
        (current_used_idx - 1) % gpu_queue_size;

    virtio_mb();

    struct virtq_used_elem used_elem =
        gpu_queue.used->ring[used_ring_index];

    if (used_elem.id != head)
    {
        puts("GPU used descriptor mismatch\n");
        return -1;
    }

    /*
     * Response 반환
     */

    for (uint32_t i = 0; i < resp_size; ++i)
    {
        ((unsigned char *)resp)[i] =
            gpu_resp_slot[slot][i];
    }

    return 0;
}

static int gpu_create_resource(void)
{
    virtio_gpu_resource_create_2d_t cmd;
    virtio_gpu_ctrl_hdr_t resp;

    memset(&cmd, 0, sizeof(cmd));
    memset(&resp, 0, sizeof(resp));

    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;

    cmd.resource_id = GPU_RESOURCE_ID;
    cmd.format = VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM;
    cmd.width = gpu_display_width;
    cmd.height = gpu_display_height;

    if (gpu_submit_control(
            &cmd,
            sizeof(cmd),
            &resp,
            sizeof(resp)) < 0)
    {
        puts("GPU create resource submit failed\n");
        return -1;
    }

    if (resp.type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump_("GPU create resource response", resp.type);
        return -1;
    }

    return 0;
}

static int gpu_attach_backing(void)
{
    // enter("gpu_attach_backing");
    unsigned char cmd_buf[sizeof(virtio_gpu_resource_attach_backing_t) +
                          sizeof(virtio_gpu_mem_entry_t)];

    virtio_gpu_resource_attach_backing_t *cmd =
        (virtio_gpu_resource_attach_backing_t *)cmd_buf;

    virtio_gpu_mem_entry_t *entry =
        (virtio_gpu_mem_entry_t *)(cmd_buf + sizeof(*cmd));

    virtio_gpu_ctrl_hdr_t resp;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    memset(&resp, 0, sizeof(resp));

    uint32_t total_bytes =
        gpu_display_width *
        gpu_display_height *
        sizeof(uint32_t);

    cmd->hdr.type =
        VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;

    cmd->hdr.flags = 0;
    cmd->hdr.fence_id = 0;
    cmd->hdr.ctx_id = 0;
    cmd->hdr.padding = 0;

    cmd->resource_id = GPU_RESOURCE_ID;
    cmd->num_entries = 1;

    entry->addr =
        (uint64_t)(uintptr_t)gpu_framebuffer;

    entry->length =
        total_bytes;

    entry->padding = 0;
    uint32_t command_size = sizeof(*cmd) + sizeof(*entry);

    if (gpu_submit_control(
            cmd_buf,
            command_size,
            &resp,
            sizeof(resp)) < 0)
    {
        puts("GPU attach backing submit failed\n");
        return -1;
    }

    if (resp.type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump_("GPU attach backing response", resp.type);
        return -1;
    }
    return 0;
}

static int gpu_set_scanout(void)
{
    virtio_gpu_set_scanout_t cmd;
    virtio_gpu_ctrl_hdr_t resp;

    memset(&cmd, 0, sizeof(cmd));
    memset(&resp, 0, sizeof(resp));

    cmd.hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;

    cmd.r.x = 0;
    cmd.r.y = 0;
    cmd.r.width = gpu_display_width;
    cmd.r.height = gpu_display_height;

    cmd.scanout_id = GPU_SCANOUT_ID;
    cmd.resource_id = GPU_RESOURCE_ID;

    if (gpu_submit_control(
            &cmd,
            sizeof(cmd),
            &resp,
            sizeof(resp)) < 0)
    {
        puts("GPU set scanout submit failed\n");
        return -1;
    }

    if (resp.type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("GPU set scanout response", resp.type);
        return -1;
    }

    return 0;
}

static int gpu_get_display_info(void)
{
    virtio_gpu_ctrl_hdr_t cmd;
    virtio_gpu_resp_display_info_t resp;

    memset(&cmd, 0, sizeof(cmd));
    memset(&resp, 0, sizeof(resp));

    cmd.type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    cmd.flags = 0;
    cmd.fence_id = 0;
    cmd.ctx_id = 0;
    cmd.padding = 0;

    if (gpu_submit_control(
            &cmd,
            sizeof(cmd),
            &resp,
            sizeof(resp)) < 0)
    {
        puts("GPU get display info submit failed\n");
        return -1;
    }

    if (resp.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO)
    {
        dump("GPU display info response", resp.hdr.type);
        return -1;
    }

    if (resp.scanout[GPU_SCANOUT_ID].enabled)
    {
        gpu_display_width =
            resp.scanout[GPU_SCANOUT_ID].rect.width;

        gpu_display_height =
            resp.scanout[GPU_SCANOUT_ID].rect.height;
    }
    else
    {
        gpu_display_width = GPU_DEFAULT_WIDTH;
        gpu_display_height = GPU_DEFAULT_HEIGHT;
    }

    dump("GPU display width", gpu_display_width);
    dump("GPU display height", gpu_display_height);

    return 0;
}

int gpu_resource_flush(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height)
{
    virtio_gpu_resource_flush_t cmd;
    virtio_gpu_ctrl_hdr_t resp;

    // enter("gpu_resource_flush");

    memset(&cmd, 0, sizeof(cmd));
    memset(&resp, 0, sizeof(resp));

    cmd.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;

    cmd.r.x = x;
    cmd.r.y = y;
    cmd.r.width = width;
    cmd.r.height = height;

    cmd.resource_id = GPU_RESOURCE_ID;
    cmd.padding = 0;

    if (gpu_submit_control(
            &cmd,
            sizeof(cmd),
            &resp,
            sizeof(resp)) < 0)
    {
        puts("GPU resource flush submit failed\n");
        return -1;
    }

    if (resp.type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("GPU resource flush response", resp.type);
        return -1;
    }
    return 0;
}

int gpu_transfer_to_host_2d(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height)
{
    // enter("gpu_transfer_to_host_2d");
    virtio_gpu_transfer_to_host_2d_t cmd;
    virtio_gpu_ctrl_hdr_t resp;

    memset(&cmd, 0, sizeof(cmd));
    memset(&resp, 0, sizeof(resp));

    cmd.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    cmd.hdr.flags = 0;
    cmd.hdr.fence_id = 0;
    cmd.hdr.ctx_id = 0;
    cmd.hdr.padding = 0;

    cmd.r.x = x;
    cmd.r.y = y;
    cmd.r.width = width;
    cmd.r.height = height;

    cmd.offset =
        ((uint64_t)y * gpu_display_width + x) *
        sizeof(uint32_t);

    cmd.resource_id = GPU_RESOURCE_ID;
    cmd.padding = 0;

    if (gpu_submit_control(
            &cmd,
            sizeof(cmd),
            &resp,
            sizeof(resp)) < 0)
    {
        puts("GPU transfer submit failed\n");
        return -1;
    }
    if (resp.type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("GPU transfer response", resp.type);
        return -1;
    }
    return 0;
}

void gpu_init(void)
{
    if (g_virtio_gpu_base == 0)
    {
        puts("GPU MMIO base not found\n");
        return;
    }

    vq_init(g_virtio_gpu_base);

    vq_setup(g_virtio_gpu_base, 0, gpu_queue_storage, &gpu_queue);

    if (gpu_get_display_info() < 0)
    {
        puts("GPU display info failed\n");
        return;
    }

    memset(
        (void *)gpu_framebuffer,
        0,
        (size_t)(gpu_display_width * gpu_display_height * sizeof(uint32_t)));

    if (gpu_create_resource() < 0)
    {
        puts("GPU create resource failed\n");
        return;
    }

    if (gpu_attach_backing() < 0)
    {
        puts("GPU attach backing failed\n");
        return;
    }

    if (gpu_set_scanout() < 0)
    {
        puts("GPU set scanout failed\n");
        return;
    }
}

void virtio_gpu_irq_handle(void)
{
}