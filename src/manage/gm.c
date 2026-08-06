#include "manage/_gm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "tools/_virtio.h"

#define GPU_RESOURCE_ID 1
#define GPU_SCANOUT_ID 0
#define GPU_DISPLAY_WIDTH 1280
#define GPU_DISPLAY_HEIGHT 720

#define GPU_CMD_SLOT_SIZE 4096
#define GPU_CMD_SLOT_COUNT 8

static unsigned char gpu_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static virtio_queue_state gpu_queue;
static uint16_t gpu_used_idx = 0;
static uint32_t gpu_queue_size = 0;

static uint32_t gpu_display_width = GPU_DISPLAY_WIDTH;
static uint32_t gpu_display_height = GPU_DISPLAY_HEIGHT;

static uint32_t gpu_framebuffer[GPU_DISPLAY_WIDTH * GPU_DISPLAY_HEIGHT]
    __attribute__((aligned(4096)));
static unsigned char gpu_cmd_buf[4096] __attribute__((aligned(4096)));
static unsigned char gpu_resp_buf[4096] __attribute__((aligned(4096)));
static unsigned char gpu_cmd_slot[4096] __attribute__((aligned(4096)));
static unsigned char gpu_resp_slot[4096] __attribute__((aligned(4096)));

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
    for (uint16_t i = gpu_used_idx; i < new_used_idx; ++i)
    {
        uint16_t used_ring_index = i % gpu_queue_size;
        virtio_mb();
        struct virtq_used_elem used_elem = gpu_queue.used->ring[used_ring_index];
        dump("used elem id", used_elem.id);
        dump("used elem len", used_elem.len);
    }

    gpu_used_idx = new_used_idx;
    return 0;
}

static void gpu_dump_status(const char *stage)
{
    dump("GPU stage", (uint64_t)stage);
    dump("GPU status", VIRTIO_GPU_STATUS);
    dump("GPU host features", VIRTIO_GPU_HOST_FEATURES);
    dump("GPU queue size", gpu_queue_size);
}

static void gpu_setup_queue(void)
{
    VIRTIO_GPU_QUEUE_SEL = 0;

    uint32_t max = VIRTIO_GPU_QUEUE_NUM_MAX;
    dump("GPU queue max supported", max);

    if (max == 0)
    {
        puts("GPU queue unavailable\n");
        return;
    }

    if (max < 2)
    {
        puts("GPU queue too small\n");
        return;
    }

    gpu_queue_size = (max < VIRTIO_QUEUE_SIZE) ? max : VIRTIO_QUEUE_SIZE;

    VIRTIO_GPU_GUEST_PAGE_SIZE = 4096;
    VIRTIO_GPU_QUEUE_ALIGN = 4096;
    VIRTIO_GPU_QUEUE_NUM = gpu_queue_size;

    gpu_queue.storage = gpu_queue_storage;
    gpu_queue.desc = (struct virtq_desc *)gpu_queue.storage;
    gpu_queue.avail = (struct virtq_avail *)(gpu_queue.storage + VIRTIO_DESC_BYTES);
    gpu_queue.used = (struct virtq_used *)(gpu_queue.storage + VIRTIO_USED_OFFSET);

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        gpu_queue.storage[i] = 0;
    }

    gpu_queue.avail->flags = 0;
    gpu_queue.avail->idx = 0;
    gpu_queue.used->flags = 0;
    gpu_queue.used->idx = 0;

    VIRTIO_GPU_QUEUE_PFN = ((uint64_t)gpu_queue.storage) >> 12;
    dump("GPU queue PFN set", VIRTIO_GPU_QUEUE_PFN);
    dump("GPU queue size", gpu_queue_size);
}

static int gpu_submit_control(uint32_t cmd_size, uint32_t resp_size)
{
    enter("gpu_submit_control");

    dump("cmd_size", cmd_size);
    dump("resp_size", resp_size);

    VIRTIO_GPU_QUEUE_SEL = 0;

    uint16_t avail_idx = gpu_queue.avail->idx;
    uint16_t ring_index = avail_idx % gpu_queue_size;

    dump("gpu submit avail idx", avail_idx);
    dump("gpu submit ring index", ring_index);
    dump("gpu submit used idx", gpu_used_idx);

    uint16_t head = (ring_index * 2) % gpu_queue_size;
    uint16_t next_desc = (head + 1) % gpu_queue_size;

    uint64_t cmd_phys = (uint64_t)(uintptr_t)gpu_cmd_slot;
    uint64_t resp_phys = (uint64_t)(uintptr_t)gpu_resp_slot;

    for (uint32_t i = 0; i < cmd_size; ++i)
        gpu_cmd_slot[i] = gpu_cmd_buf[i];
    for (uint32_t i = cmd_size; i < sizeof(gpu_cmd_slot); ++i)
        gpu_cmd_slot[i] = 0;

    for (uint32_t i = 0; i < resp_size; ++i)
        gpu_resp_slot[i] = 0;
    for (uint32_t i = resp_size; i < sizeof(gpu_resp_slot); ++i)
        gpu_resp_slot[i] = 0;

    gpu_queue.desc[head].addr = cmd_phys;
    gpu_queue.desc[head].len = cmd_size;
    gpu_queue.desc[head].flags = VIRTQ_DESC_F_NEXT;
    gpu_queue.desc[head].next = next_desc;

    gpu_queue.desc[next_desc].addr = resp_phys;
    gpu_queue.desc[next_desc].len = resp_size;
    gpu_queue.desc[next_desc].flags = VIRTQ_DESC_F_WRITE;
    gpu_queue.desc[next_desc].next = 0;

    gpu_queue.avail->ring[ring_index] = head;
    virtio_mb();
    gpu_queue.avail->idx = avail_idx + 1;
    virtio_mb();

    VIRTIO_GPU_QUEUE_NOTIFY = 0;

    if (gpu_wait_used() < 0)
        return -1;

    // 디바이스가 최종 기록한 used 인덱스를 기준으로 모듈러 연산 수행
    uint16_t current_used_idx = gpu_queue.used->idx;
    uint16_t used_ring_index = (current_used_idx - 1) % VIRTIO_QUEUE_SIZE;

    virtio_mb();
    struct virtq_used_elem used_elem = gpu_queue.used->ring[used_ring_index];

    if (used_elem.id != head)
        return -1;

    uint32_t copy_sz = resp_size;
    if (copy_sz > sizeof(gpu_resp_buf))
        copy_sz = sizeof(gpu_resp_buf);
    for (uint32_t i = 0; i < copy_sz; ++i)
        gpu_resp_buf[i] = gpu_resp_slot[i];

    return 0;
}

static int gpu_create_resource(void)
{
    enter("gpu_create_resource");

    virtio_gpu_resource_create_2d_t *cmd =
        (virtio_gpu_resource_create_2d_t *)gpu_cmd_buf;

    memset(gpu_cmd_buf, 0, sizeof(gpu_cmd_buf));
    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));

    cmd->hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    cmd->hdr.flags = 0;
    cmd->hdr.fence_id = 0;
    cmd->hdr.ctx_id = 0;
    cmd->hdr.padding = 0;

    cmd->resource_id = GPU_RESOURCE_ID;
    cmd->format = VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM;
    cmd->width = gpu_display_width;
    cmd->height = gpu_display_height;

    int ret = gpu_submit_control(
        sizeof(*cmd),
        sizeof(gpu_resp_buf));

    if (ret < 0)
    {
        puts("gpu_create_resource command failed\n");
        return -1;
    }

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("create response", resp->type);
    if (resp->type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("create resource failed type", resp->type);
        full_stop();
        return -1;
    }

    return 0;
}

static int gpu_attach_backing(void)
{
    enter("gpu_attach_backing");

    memset(gpu_cmd_buf, 0, sizeof(gpu_cmd_buf));
    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));

    uint32_t total_bytes = gpu_display_width * gpu_display_height * sizeof(uint32_t);

    dump("sizeof framebuffer", sizeof(gpu_framebuffer));
    dump("attach total bytes", total_bytes);
    dump("fb address", (uint64_t)gpu_framebuffer);

    virtio_gpu_resource_attach_backing_t *cmd = (virtio_gpu_resource_attach_backing_t *)gpu_cmd_buf;
    cmd->hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    cmd->hdr.flags = 0;
    cmd->hdr.fence_id = 0;
    cmd->hdr.ctx_id = 0;
    cmd->hdr.padding = 0;
    cmd->resource_id = GPU_RESOURCE_ID;
    cmd->num_entries = 1;

    // 헤더 바로 뒤에 첫 번째이자 유일한 mem_entry 배치
    virtio_gpu_mem_entry_t *entry = (virtio_gpu_mem_entry_t *)(gpu_cmd_buf + sizeof(*cmd));
    entry[0].addr = (uint64_t)(uintptr_t)gpu_framebuffer;
    entry[0].length = total_bytes;
    entry[0].padding = 0;

    uint32_t command_size = sizeof(virtio_gpu_resource_attach_backing_t) + sizeof(virtio_gpu_mem_entry_t);

    dump("attach command size", command_size);
    // Debug: dump first few 64-bit words of the command buffer to see exact bytes sent
    for (int i = 0; i < 8; ++i)
    {
        uint64_t *w = (uint64_t *)(gpu_cmd_buf + (i * 8));
        dump("cmd buf word", *w);
    }

    dump("attach mem_entry addr", entry[0].addr);
    dump("attach mem_entry len", entry[0].length);

    int ret = gpu_submit_control(command_size, sizeof(gpu_resp_buf));
    if (ret < 0)
    {
        puts("gpu_submit_control failed\n");
        return -1;
    }

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("attach backing resp type", resp->type);
    if (resp->type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("attach backing failed type", resp->type);
        full_stop();
        return -1;
    }

    return 0;
}

static int gpu_set_scanout(void)
{
    enter("gpu_set_scanout");

    virtio_gpu_set_scanout_t *cmd = (virtio_gpu_set_scanout_t *)gpu_cmd_buf;

    memset(gpu_cmd_buf, 0, sizeof(gpu_cmd_buf));
    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));

    // 1. 헤더 설정
    cmd->hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
    cmd->hdr.flags = 0;
    cmd->hdr.fence_id = 0;
    cmd->hdr.ctx_id = 0;
    cmd->hdr.padding = 0;

    // 2. 값 대입
    cmd->r.x = 0;
    cmd->r.y = 0;
    cmd->r.width = gpu_display_width;
    cmd->r.height = gpu_display_height;
    cmd->scanout_id = GPU_SCANOUT_ID;   // 반드시 0인지 확인!
    cmd->resource_id = GPU_RESOURCE_ID; // 반드시 1인지 확인!

    dump("cmd->resource_id", cmd->resource_id);
    dump("cmd->scanout_id", cmd->scanout_id);
    dump("cmd->r.width", cmd->r.width);
    dump("cmd->r.height", cmd->r.height);
    dump_("sizeof(*cmd)", sizeof(*cmd));
    dump("cmd->r.x", cmd->r.x);
    dump("cmd->r.y", cmd->r.y);

    int ret = gpu_submit_control(52, sizeof(gpu_resp_buf));
    //     int ret = gpu_submit_control(sizeof(*cmd), sizeof(gpu_resp_buf));
    if (ret < 0)
    {
        puts("gpu_set_scanout command failed\n");
        return -1;
    }

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;

    dump("scanout response", resp->type);
    if (resp->type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("set scanout failed type", resp->type);
        full_stop();
        return -1;
    }
    exit("gpu_set_scanout");
    return 0;
}

static int gpu_get_display_info(void)
{
    enter("gpu_get_display_info");
    virtio_gpu_ctrl_hdr_t *cmd = (virtio_gpu_ctrl_hdr_t *)gpu_cmd_buf;
    cmd->type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    cmd->flags = 0;
    cmd->fence_id = 0;
    cmd->ctx_id = 0;
    cmd->padding = 0;

    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));
    if (gpu_submit_control(sizeof(*cmd), sizeof(gpu_resp_buf)) < 0)
    {
        puts("gpu_get_display_info command failed\n");
        return -1;
    }

    virtio_gpu_resp_display_info_t *resp = (virtio_gpu_resp_display_info_t *)gpu_resp_buf;
    if (resp->hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO)
    {
        dump("get display info failed type", resp->hdr.type);
        return -1;
    }

    dump("GET_DISPLAY_INFO resp type", resp->hdr.type);
    dump("GET_DISPLAY_INFO scanout enabled", resp->scanout[0].enabled);
    dump("GET_DISPLAY_INFO scanout x", resp->scanout[0].rect.x);
    dump("GET_DISPLAY_INFO scanout y", resp->scanout[0].rect.y);
    dump("GET_DISPLAY_INFO scanout width", resp->scanout[0].rect.width);
    dump("GET_DISPLAY_INFO scanout height", resp->scanout[0].rect.height);
    dump("GET_DISPLAY_INFO scanout flags", resp->scanout[0].flags);

    if (resp->scanout[0].enabled == 0)
    {
        gpu_display_width = GPU_DISPLAY_WIDTH;
        gpu_display_height = GPU_DISPLAY_HEIGHT;
    }
    else
    {
        gpu_display_width = resp->scanout[0].rect.width;
        gpu_display_height = resp->scanout[0].rect.height;
    }

    dump("GPU display width", gpu_display_width);
    dump("GPU display height", gpu_display_height);
    return 0;
}

static int gpu_resource_flush(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    virtio_gpu_resource_flush_t *cmd = (virtio_gpu_resource_flush_t *)gpu_cmd_buf;
    cmd->hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    cmd->hdr.flags = 0;
    cmd->hdr.fence_id = 0;
    cmd->hdr.ctx_id = 0;
    cmd->hdr.padding = 0;
    cmd->resource_id = GPU_RESOURCE_ID;
    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;

    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));
    int ret = gpu_submit_control(sizeof(*cmd), sizeof(gpu_resp_buf));
    if (ret < 0)
    {
        puts("gpu_resource_flush command failed\n");
        return -1;
    }

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("flush response", resp->type);

    if (resp->type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("flush failed type", resp->type);
        full_stop();
        return -1;
    }
    return 0;
}

static int gpu_transfer_to_host_2d(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    enter("gpu_transfer_to_host_2d");
    virtio_gpu_transfer_to_host_2d_t *cmd = (virtio_gpu_transfer_to_host_2d_t *)gpu_cmd_buf;
    cmd->hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    cmd->hdr.flags = 0;
    cmd->hdr.fence_id = 0;
    cmd->hdr.ctx_id = 0;
    cmd->hdr.padding = 0;

    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    cmd->offset = (uint64_t)((uint64_t)y * gpu_display_width + x) * sizeof(uint32_t);
    cmd->resource_id = GPU_RESOURCE_ID;
    cmd->padding = 0;

    dump_("sizeof(*cmd)", sizeof(*cmd));

    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));
    int ret = gpu_submit_control(sizeof(*cmd), sizeof(gpu_resp_buf));
    if (ret < 0)
    {
        puts("gpu_transfer_to_host_2d command failed\n");
        return -1;
    }

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;

    dump("transfer response", resp->type);
    if (resp->type != VIRTIO_GPU_RESP_OK_NODATA)
    {
        dump("transfer failed type", resp->type);
        full_stop();
        return -1;
    }
    return 0;
}

void gpu_init(void)
{
    dump("fb addr:", (uint64_t)gpu_framebuffer);

    if (g_virtio_gpu_base == 0)
    {
        puts("GPU MMIO base not found\n");
        return;
    }

    // Device reset
    VIRTIO_GPU_STATUS = 0;

    // Driver acknowledge
    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_DRIVER;

    VIRTIO_GPU_HOST_FEATURES_SEL = 0;

    uint32_t host_features = VIRTIO_GPU_HOST_FEATURES;

    dump("GPU host features", host_features);

    VIRTIO_GPU_GUEST_FEATURES_SEL = 0;
    VIRTIO_GPU_GUEST_FEATURES = 0;

    // legacy queue 설정용
    VIRTIO_GPU_GUEST_PAGE_SIZE = 4096;

    // Feature negotiation 완료
    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_FEATURES_OK;

    if ((VIRTIO_GPU_STATUS & VIRTIO_STATUS_FEATURES_OK) == 0)
    {
        puts("GPU FEATURES_OK rejected\n");
        return;
    }

    // Virtqueue 초기화
    gpu_setup_queue();

    // Driver ready
    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    gpu_dump_status("after driver ok");

    puts("GPU driver initialized\n");

    if (gpu_get_display_info() < 0)
    {
        puts("GPU display info failed\n");
        return;
    }

    memset(gpu_framebuffer, 0, sizeof(gpu_framebuffer));

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

    if (gpu_resource_flush(
            0,
            0,
            gpu_display_width,
            gpu_display_height) < 0)
    {
        puts("GPU resource flush failed\n");
        return;
    }

    puts("GPU display ready\n");
}

void draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= GPU_DISPLAY_WIDTH || y >= GPU_DISPLAY_HEIGHT)
        return;

    gpu_framebuffer[y * GPU_DISPLAY_WIDTH + x] = color;
    gpu_transfer_to_host_2d(x, y, 1, 1);
    gpu_resource_flush(x, y, 1, 1);
}

void gpu_fill_screen(uint32_t color)
{
    uint32_t pixel_count = GPU_DISPLAY_WIDTH * GPU_DISPLAY_HEIGHT;
    for (uint32_t i = 0; i < pixel_count; ++i)
    {
        gpu_framebuffer[i] = color;
    }
    gpu_transfer_to_host_2d(0, 0, GPU_DISPLAY_WIDTH, GPU_DISPLAY_HEIGHT);
    gpu_resource_flush(0, 0, GPU_DISPLAY_WIDTH, GPU_DISPLAY_HEIGHT);
}

void gpu_test(void)
{
    draw_pixel(10, 10, 0xFFFF0000);
    draw_pixel(20, 20, 0xFF00FF00);
    draw_pixel(30, 30, 0xFF0000FF);
    puts("GPU test pixels written\n");
}
