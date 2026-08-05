#include "manage/_gm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "tools/_virtio.h"

#define GPU_RESOURCE_ID 1
#define GPU_SCANOUT_ID 0
#define GPU_DISPLAY_WIDTH 1280
#define GPU_DISPLAY_HEIGHT 720

static unsigned char gpu_queue_storage[VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static virtio_queue_state gpu_queue;
static uint16_t gpu_used_idx = 0;

static uint32_t gpu_display_width = GPU_DISPLAY_WIDTH;
static uint32_t gpu_display_height = GPU_DISPLAY_HEIGHT;

static uint32_t gpu_framebuffer[GPU_DISPLAY_WIDTH * GPU_DISPLAY_HEIGHT]
    __attribute__((aligned(4096)));
static unsigned char gpu_cmd_buf[65536] __attribute__((aligned(4096)));
static unsigned char gpu_resp_buf[4096] __attribute__((aligned(4096)));

static int gpu_wait_used(void)
{
    int timeout = 10000000;
    while (gpu_queue.used->idx == gpu_used_idx)
    {
        if (--timeout == 0)
        {
            puts("GPU queue timed out\n");
            return -1;
        }
    }

    // ★ 디바이스가 발생시킨 인터럽트 플래그 읽고 클리어하기
    uint32_t irq_status = VIRTIO_GPU_INTERRUPT_STATUS;
    if (irq_status != 0)
    {
        VIRTIO_GPU_INTERRUPT_ACK = irq_status;
    }

    gpu_used_idx = gpu_queue.used->idx;
    return 0;
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

    if (max < VIRTIO_QUEUE_SIZE)
    {
        puts("GPU queue too small\n");
        return;
    }

    VIRTIO_GPU_GUEST_PAGE_SIZE = 4096;
    VIRTIO_GPU_QUEUE_ALIGN = 4096;
    VIRTIO_GPU_QUEUE_NUM = VIRTIO_QUEUE_SIZE;

    gpu_queue.storage = gpu_queue_storage;
    gpu_queue.desc = (struct virtq_desc *)gpu_queue.storage;
    gpu_queue.avail = (struct virtq_avail *)(gpu_queue.storage + VIRTIO_DESC_BYTES);
    gpu_queue.used = (struct virtq_used *)(gpu_queue.storage + VIRTIO_USED_OFFSET);

    for (uint32_t i = 0; i < VIRTIO_QUEUE_STORAGE; ++i)
    {
        gpu_queue.storage[i] = 0;
    }

    VIRTIO_GPU_QUEUE_PFN = ((uint64_t)gpu_queue.storage) >> 12;
    dump("GPU queue PFN set", VIRTIO_GPU_QUEUE_PFN);
}

static int gpu_submit_control(uint32_t cmd_size, uint32_t resp_size)
{
    enter("gpu_submit_control");
    VIRTIO_GPU_QUEUE_SEL = 0;

    // avail_idx를 기반으로 디스크립터 슬롯을 순환하며 할당
    // (보통 제어 요청은 명령(Read) + 응답(Write) 2개의 디스크립터를 한 쌍으로 묶어 사용)
    uint16_t avail_idx = gpu_queue.avail->idx;
    uint16_t ring_index = avail_idx % VIRTIO_QUEUE_SIZE;

    // 디스크립터 테이블에서 2개씩 쌍을 지어 순환하도록 계산 (0-1, 2-3, 4-5 ...)
    uint16_t head = (avail_idx * 2) % VIRTIO_QUEUE_SIZE;
    uint16_t next_desc = (head + 1) % VIRTIO_QUEUE_SIZE;

    // 첫 번째 디스크립터 설정 (Command - Read Only)
    gpu_queue.desc[head].addr = (uint64_t)gpu_cmd_buf;
    gpu_queue.desc[head].len = cmd_size;
    gpu_queue.desc[head].flags = VIRTQ_DESC_F_NEXT;
    gpu_queue.desc[head].next = next_desc;

    // 두 번째 디스크립터 설정 (Response - Write Only)
    gpu_queue.desc[next_desc].addr = (uint64_t)gpu_resp_buf;
    gpu_queue.desc[next_desc].len = resp_size;
    gpu_queue.desc[next_desc].flags = VIRTQ_DESC_F_WRITE;
    gpu_queue.desc[next_desc].next = 0;

    // Avail 링에 이번 요청의 헤드 번호 등록
    gpu_queue.avail->ring[ring_index] = head;

    dump("ring_index", (uint64_t)ring_index);
    dump("ring_value_written", (uint64_t)head); // 이제 0, 2, 4, 6... 순으로 다이내믹하게 찍혀야 함!

    virtio_mb();
    gpu_queue.avail->idx++;
    virtio_mb();

    dump("avail_idx_after", (uint64_t)gpu_queue.avail->idx);

    // QEMU에 알림 전송 (Queue Index 0)
    VIRTIO_GPU_QUEUE_NOTIFY = 0;

    if (gpu_wait_used() < 0)
    {
        dump("gpu_wait_used_error", 1);
        return -1;
    }

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

    dump(
        "create response",
        ((virtio_gpu_ctrl_hdr_t *)gpu_resp_buf)->type);

    return ret;
}
static int gpu_attach_backing(void)
{
    enter("gpu_attach_backing");

    memset(gpu_cmd_buf, 0, sizeof(gpu_cmd_buf));
    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));

    uint32_t total_bytes = gpu_display_width * gpu_display_height * sizeof(uint32_t);

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

    int ret = gpu_submit_control(command_size, sizeof(gpu_resp_buf));
    if (ret < 0)
    {
        puts("gpu_submit_control failed\n");
        return ret;
    }

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("resp type", resp->type);

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
    cmd->scanout_id = GPU_SCANOUT_ID;   // 반드시 0인지 확인!
    cmd->resource_id = GPU_RESOURCE_ID; // 반드시 1인지 확인!
    cmd->x = 0;
    cmd->y = 0;
    cmd->width = gpu_display_width;
    cmd->height = gpu_display_height;

    dump("SET_SCANOUT cmd size", sizeof(*cmd));

    // ★ 여기서 크기를 정확히 sizeof(*cmd) (즉, 48바이트 = 0x30)로 넘겨야 합니다.
    int ret = gpu_submit_control(sizeof(*cmd), sizeof(gpu_resp_buf));

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("scanout response", resp->type); // 이 값이 반드시 0x00(OK)이 되어야 합니다!

    return ret;
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
        return -1;
    }

    virtio_gpu_resp_display_info_t *resp = (virtio_gpu_resp_display_info_t *)gpu_resp_buf;

    if (resp->scanout[0].enabled == 0)
    {
        puts("GPU scanout is not active, using default resolution\n");
        gpu_display_width = GPU_DISPLAY_WIDTH;
        gpu_display_height = GPU_DISPLAY_HEIGHT;
    }
    else
    {
        gpu_display_width = resp->scanout[0].rect.width;
        gpu_display_height = resp->scanout[0].rect.height;
    }

    dump("resp type", ((virtio_gpu_ctrl_hdr_t *)gpu_resp_buf)->type);

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

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("flush response", resp->type);
    return ret;
}

static int gpu_transfer_to_host_2d(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
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
    cmd->offset = 0;
    cmd->resource_id = GPU_RESOURCE_ID;
    cmd->padding = 0;

    memset(gpu_resp_buf, 0, sizeof(gpu_resp_buf));
    int ret = gpu_submit_control(sizeof(*cmd), sizeof(gpu_resp_buf));

    virtio_gpu_ctrl_hdr_t *resp = (virtio_gpu_ctrl_hdr_t *)gpu_resp_buf;
    dump("transfer response", resp->type);
    return ret;
}

void gpu_init(void)
{
    dump("fb addr:", gpu_framebuffer);

    if (g_virtio_gpu_base == 0)
    {
        puts("GPU MMIO base not found\n");
        return;
    }

    VIRTIO_GPU_STATUS = 0;
    VIRTIO_GPU_STATUS = VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_DRIVER;

    uint32_t host_features = VIRTIO_GPU_HOST_FEATURES;
    dump("GPU host features", host_features);

    VIRTIO_GPU_GUEST_FEATURES = 0;
    VIRTIO_GPU_GUEST_PAGE_SIZE = 4096;

    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_FEATURES_OK;
    gpu_setup_queue();

    if (gpu_get_display_info() < 0)
    {
        puts("GPU display info failed\n");
        return;
    }

    VIRTIO_GPU_STATUS |= VIRTIO_STATUS_DRIVER_OK;

    puts("GPU driver initialized\n");

    memset(gpu_framebuffer, 0, sizeof(gpu_framebuffer));
    gpu_create_resource();

    gpu_attach_backing();

    gpu_set_scanout();
    gpu_resource_flush(0, 0, gpu_display_width, gpu_display_height);
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
