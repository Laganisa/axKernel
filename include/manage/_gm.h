#ifndef __KERNEL_GM_H__
#define __KERNEL_GM_H__

#include "_defs.h"
#include "_types.h"
#include "_macro.h"

#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO 0x0100
#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D 0x0101
#define VIRTIO_GPU_CMD_RESOURCE_UNREF 0x0102
#define VIRTIO_GPU_CMD_SET_SCANOUT 0x0103
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D 0x0104
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 0x0105
#define VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING 0x0106
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH 0x0107

#define VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM 1

typedef struct __attribute__((packed)) virtio_gpu_ctrl_hdr
{
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint32_t padding;
} virtio_gpu_ctrl_hdr_t;

typedef struct __attribute__((packed)) virtio_gpu_resource_create_2d
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} virtio_gpu_resource_create_2d_t;

typedef struct __attribute__((packed)) virtio_gpu_resource_attach_backing
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t num_entries;

} virtio_gpu_resource_attach_backing_t;

typedef struct __attribute__((packed)) virtio_gpu_mem_entry
{
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} virtio_gpu_mem_entry_t;

typedef struct __attribute__((packed)) virtio_gpu_resource_flush
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} virtio_gpu_resource_flush_t;

typedef struct __attribute__((packed)) virtio_gpu_transfer_to_host_2d
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint64_t offset;
    uint32_t resource_id;
    uint32_t padding;
} virtio_gpu_transfer_to_host_2d_t;

typedef struct __attribute__((packed)) virtio_gpu_rect
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} virtio_gpu_rect_t;

/*
typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t scanout_id;
    uint32_t resource_id;
    virtio_gpu_rect_t r;
} virtio_gpu_set_scanout_t;
*/

typedef struct __attribute__((packed))
{
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t scanout_id;
    uint32_t resource_id;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} virtio_gpu_set_scanout_t;

typedef struct __attribute__((packed)) virtio_gpu_display_one
{
    virtio_gpu_rect_t rect;
    uint32_t enabled;
    uint32_t flags;
} virtio_gpu_display_one_t;

typedef struct __attribute__((packed)) virtio_gpu_resp_display_info
{
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_display_one_t scanout[1];
} virtio_gpu_resp_display_info_t;

void gpu_init(void);
void draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void gpu_fill_screen(uint32_t color);
void gpu_test(void);

#endif