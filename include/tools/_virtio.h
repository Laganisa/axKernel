
#ifndef __KERNEL_VIRTIO_H__
#define __KERNEL_VIRTIO_H__

#include "_types.h"
#include "_macro.h"
#include "_defs.h"

struct virtq_desc
{
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct virtq_avail
{
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[256];
};

struct virtq_used_elem
{
    uint32_t id;
    uint32_t len;
};

struct virtq_used
{
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[256];
};

typedef struct virtio_queue_state
{
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
    unsigned char *storage;
} virtio_queue_state;

extern virtio_queue_state rx_queue;
extern virtio_queue_state tx_queue;

#define VIRTIO_QUEUE_SIZE 256

#define VIRTIO_DESC_BYTES (16 * VIRTIO_QUEUE_SIZE)
#define VIRTIO_AVAIL_BYTES (4 + 2 * VIRTIO_QUEUE_SIZE)
#define VIRTIO_USED_BYTES (4 + 8 * VIRTIO_QUEUE_SIZE)

#define ALIGN_4K(val) (((val) + 4095) & ~4095)

#define VIRTIO_USED_OFFSET ALIGN_4K(VIRTIO_DESC_BYTES + VIRTIO_AVAIL_BYTES)
#define VIRTIO_QUEUE_BYTES (VIRTIO_USED_OFFSET + VIRTIO_USED_BYTES)
#define VIRTIO_QUEUE_STORAGE ALIGN_4K(VIRTIO_QUEUE_BYTES)

extern uint16_t last_rx_used_idx;
extern uint16_t last_tx_used_idx;

static inline void virtio_mb(void)
{
    __asm__ volatile("dmb ish" ::: "memory");
}

void *get_ring_buffer_addr(void);

void setup_virtqueue(int queue_index);

#define VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM 1

// qemu 8.2.2 에서 가져온 상수 정의
enum virtio_gpu_ctrl_type
{
    VIRTIO_GPU_UNDEFINED = 0,

    /* 2d commands */
    VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
    VIRTIO_GPU_CMD_RESOURCE_UNREF,
    VIRTIO_GPU_CMD_SET_SCANOUT,
    VIRTIO_GPU_CMD_RESOURCE_FLUSH,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
    VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
    VIRTIO_GPU_CMD_GET_CAPSET_INFO,
    VIRTIO_GPU_CMD_GET_CAPSET,
    VIRTIO_GPU_CMD_GET_EDID,
    VIRTIO_GPU_CMD_RESOURCE_ASSIGN_UUID,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_BLOB,
    VIRTIO_GPU_CMD_SET_SCANOUT_BLOB,

    /* 3d commands */
    VIRTIO_GPU_CMD_CTX_CREATE = 0x0200,
    VIRTIO_GPU_CMD_CTX_DESTROY,
    VIRTIO_GPU_CMD_CTX_ATTACH_RESOURCE,
    VIRTIO_GPU_CMD_CTX_DETACH_RESOURCE,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_3D,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_3D,
    VIRTIO_GPU_CMD_TRANSFER_FROM_HOST_3D,
    VIRTIO_GPU_CMD_SUBMIT_3D,
    VIRTIO_GPU_CMD_RESOURCE_MAP_BLOB,
    VIRTIO_GPU_CMD_RESOURCE_UNMAP_BLOB,

    /* cursor commands */
    VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
    VIRTIO_GPU_CMD_MOVE_CURSOR,

    /* success responses */
    VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
    VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET,
    VIRTIO_GPU_RESP_OK_EDID,
    VIRTIO_GPU_RESP_OK_RESOURCE_UUID,
    VIRTIO_GPU_RESP_OK_MAP_INFO,

    /* error responses */
    VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
    VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
    VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

typedef struct __attribute__((packed))
{
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint32_t padding;
} virtio_gpu_ctrl_hdr_t;

typedef struct __attribute__((packed))
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} virtio_gpu_rect_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} virtio_gpu_resource_create_2d_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t num_entries;
} virtio_gpu_resource_attach_backing_t;

typedef struct __attribute__((packed))
{
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} virtio_gpu_mem_entry_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint64_t offset;
    uint32_t resource_id;
    uint32_t padding;
} virtio_gpu_transfer_to_host_2d_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint32_t resource_id;
    uint32_t padding;
} virtio_gpu_resource_flush_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint32_t scanout_id;
    uint32_t resource_id;
} virtio_gpu_set_scanout_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_rect_t rect;
    uint32_t enabled;
    uint32_t flags;
} virtio_gpu_display_one_t;

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_display_one_t scanout[16];
} virtio_gpu_resp_display_info_t;

extern uint32_t gpu_display_width;
extern uint32_t gpu_display_height;
extern volatile uint32_t *gpu_framebuffer;

void gpu_init(void);

int gpu_transfer_to_host_2d(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height);

int gpu_resource_flush(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height);

void blk_init(void);

#endif