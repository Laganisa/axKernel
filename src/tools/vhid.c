#include "_macro.h"
#include "global/_debug.h"
#include "tools/_virtio.h"
#include "global/_io.h"

#define VIRTIO_INPUT_EVENT_QUEUE 0
#define VIRTIO_INPUT_STATUS_QUEUE 1
#define VIRTIO_STATUS_FAILED 0x80U
#define VIRTIO_INPUT_EVENT_BUFFER_COUNT VIRTIO_QUEUE_SIZE

struct virtio_input_wire_event
{
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed));

struct virtio_input_device
{
    uint64_t base;
    virtio_queue_state queues[2];
    struct virtio_input_wire_event event_buffers[VIRTIO_INPUT_EVENT_BUFFER_COUNT];
    uint16_t last_used_idx;
};

static uint8_t input_queue_storage[VIRTIO_INPUT_MAX_DEVICES][2][VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static struct virtio_input_device input_devices[VIRTIO_INPUT_MAX_DEVICES];
static uint32_t next_input_device;

static void input_mark_failed(uint64_t base)
{
    VIRTIO_TOT_REG(base, 0x070) |= VIRTIO_STATUS_FAILED;
}

static int input_setup_event_queue(struct virtio_input_device *device)
{
    virtio_queue_state *queue = &device->queues[VIRTIO_INPUT_EVENT_QUEUE];

    if (queue->size > VIRTIO_INPUT_EVENT_BUFFER_COUNT)
    {
        puts("VirtIO input event queue exceeds buffer capacity\n");
        return -1;
    }

    for (uint16_t i = 0; i < queue->size; ++i)
    {
        queue->desc[i].addr =
            (uint64_t)(uintptr_t)&device->event_buffers[i];
        queue->desc[i].len = sizeof(device->event_buffers[i]);
        queue->desc[i].flags = VIRTQ_DESC_F_WRITE;
        queue->desc[i].next = 0;
        queue->avail->ring[i] = i;
    }

    virtio_mb();
    queue->avail->idx = queue->size;
    return 0;
}

void vhid_init(void)
{
    if (g_virtio_input_count == 0)
    {
        puts("VirtIO input devices not found\n");
        return;
    }

    for (uint32_t i = 0; i < g_virtio_input_count; ++i)
    {
        uint64_t base = g_virtio_input_bases[i];
        uint32_t version = virtio_mmio_version(base);
        dump("VirtIO input MMIO version", version);

        if (version != 1)
        {
            puts("VirtIO input requires legacy MMIO version 1\n");
            input_mark_failed(base);
            continue;
        }

        struct virtio_input_device *device = &input_devices[i];
        device->base = base;
        VIRTIO_TOT_REG(base, 0x070) = 0;
        vq_init(base);

        if (vq_setup_v1(
                base,
                VIRTIO_INPUT_EVENT_QUEUE,
                input_queue_storage[i][VIRTIO_INPUT_EVENT_QUEUE],
                &device->queues[VIRTIO_INPUT_EVENT_QUEUE]) < 0 ||
            vq_setup_v1(
                base,
                VIRTIO_INPUT_STATUS_QUEUE,
                input_queue_storage[i][VIRTIO_INPUT_STATUS_QUEUE],
                &device->queues[VIRTIO_INPUT_STATUS_QUEUE]) < 0 ||
            input_setup_event_queue(device) < 0)
        {
            input_mark_failed(base);
            continue;
        }

        device->last_used_idx = 0;
        vq_start_v1(base);
        virtio_mmio_notify_queue(base, VIRTIO_INPUT_EVENT_QUEUE);
        dump("VirtIO input ready", i);
    }
}

int vhid_get_event(virtio_input_event_t *event, uint32_t *device_index)
{
    if (event == NULL || device_index == NULL)
    {
        puts("VirtIO input event output is null\n");
        return -1;
    }

    if (g_virtio_input_count == 0)
    {
        return 0;
    }

    for (uint32_t offset = 0; offset < g_virtio_input_count; ++offset)
    {
        uint32_t index = (next_input_device + offset) % g_virtio_input_count;
        struct virtio_input_device *device = &input_devices[index];
        virtio_queue_state *queue = &device->queues[VIRTIO_INPUT_EVENT_QUEUE];

        if (queue->size == 0)
        {
            continue;
        }

        virtio_mb();
        if (queue->used->idx == device->last_used_idx)
        {
            continue;
        }

        struct virtq_used_elem *used =
            &queue->used->ring[device->last_used_idx % queue->size];
        uint32_t descriptor_id = used->id;
        uint32_t event_len = used->len;
        device->last_used_idx++;

        if (descriptor_id >= queue->size)
        {
            dump("VirtIO input invalid descriptor", descriptor_id);
            next_input_device = (index + 1) % g_virtio_input_count;
            return -1;
        }

        struct virtio_input_wire_event *wire_event =
            &device->event_buffers[descriptor_id];
        int event_is_valid = event_len >= sizeof(*wire_event);
        if (event_is_valid)
        {
            event->type = wire_event->type;
            event->code = wire_event->code;
            event->value = wire_event->value;
            *device_index = index;
        }

        uint16_t avail_idx = queue->avail->idx;
        queue->avail->ring[avail_idx % queue->size] = (uint16_t)descriptor_id;
        virtio_mb();
        queue->avail->idx = avail_idx + 1;
        virtio_mb();
        virtio_mmio_notify_queue(device->base, VIRTIO_INPUT_EVENT_QUEUE);

        next_input_device = (index + 1) % g_virtio_input_count;
        if (!event_is_valid)
        {
            puts("VirtIO input event is truncated\n");
            return -1;
        }
        return 1;
    }

    return 0;
}