#include "_macro.h"
#include "global/_debug.h"
#include "tools/_virtio.h"
#include "global/_io.h"
#include "handler/_gic.h"

#define VIRTIO_INPUT_EVENT_QUEUE 0
#define VIRTIO_INPUT_STATUS_QUEUE 1
#define VIRTIO_STATUS_FAILED 0x80U
#define VIRTIO_INPUT_EVENT_BUFFER_COUNT VIRTIO_QUEUE_SIZE
#define VIRTIO_INPUT_CONFIG 0x100
#define VIRTIO_INPUT_CFG_SELECT 0
#define VIRTIO_INPUT_CFG_SUBSELECT 1
#define VIRTIO_INPUT_CFG_SIZE 2
#define VIRTIO_INPUT_CFG_DATA 8
#define VIRTIO_INPUT_CFG_DATA_MAX 128
#define VIRTIO_INPUT_CFG_ID_NAME 1
#define VIRTIO_INPUT_CFG_EV_BITS 0x11
#define VIRTIO_INPUT_EV_KEY 1
#define VIRTIO_INPUT_EV_REL 2
#define VIRTIO_INPUT_EV_ABS 3
#define VIRTIO_INPUT_KEY_ESC 1
#define VIRTIO_INPUT_KEY_ENTER 28
#define VIRTIO_INPUT_KEY_LEFTCTRL 29
#define VIRTIO_INPUT_KEY_A 30
#define VIRTIO_INPUT_KEY_F1 59
#define VIRTIO_INPUT_KEY_F10 68
#define VIRTIO_INPUT_KEY_F11 87
#define VIRTIO_INPUT_KEY_F12 88
#define VIRTIO_INPUT_KEY_RIGHTCTRL 97
#define VIRTIO_INPUT_KEY_UP 103
#define VIRTIO_INPUT_KEY_LEFT 105
#define VIRTIO_INPUT_KEY_RIGHT 106
#define VIRTIO_INPUT_KEY_DOWN 108
#define VIRTIO_INPUT_BTN_LEFT 272
#define VIRTIO_INPUT_REL_X 0
#define VIRTIO_INPUT_REL_Y 1
#define VIRTIO_INPUT_ABS_X 0
#define VIRTIO_INPUT_ABS_Y 1
#define VIRTIO_INPUT_KEY_LEFTSHIFT 42
#define VIRTIO_INPUT_KEY_RIGHTSHIFT 54
#define VIRTIO_INPUT_KEY_CAPSLOCK 58
#define VIRTIO_INPUT_CHAR_QUEUE_SIZE 256

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
    uint8_t left_shift;
    uint8_t right_shift;
    uint8_t caps_lock;
};

static uint8_t input_queue_storage[VIRTIO_INPUT_MAX_DEVICES][2][VIRTIO_QUEUE_STORAGE]
    __attribute__((aligned(4096)));
static struct virtio_input_device input_devices[VIRTIO_INPUT_MAX_DEVICES];
static uint32_t next_input_device;
static int8_t input_char_queue[VIRTIO_INPUT_CHAR_QUEUE_SIZE];
static volatile uint32_t input_char_head;
static volatile uint32_t input_char_tail;
static volatile uint32_t input_char_dropped;
static uint32_t input_char_dropped_reported;

static uint8_t input_config_read(uint64_t base, uint32_t offset)
{
    return *(volatile uint8_t *)(uintptr_t)(base + VIRTIO_INPUT_CONFIG + offset);
}

static void input_config_write(uint64_t base, uint32_t offset, uint8_t value)
{
    *(volatile uint8_t *)(uintptr_t)(base + VIRTIO_INPUT_CONFIG + offset) = value;
}

static int input_read_config_data(
    uint64_t base,
    uint8_t select,
    uint8_t subselect,
    uint8_t *data,
    uint8_t *size)
{
    input_config_write(base, VIRTIO_INPUT_CFG_SELECT, select);
    input_config_write(base, VIRTIO_INPUT_CFG_SUBSELECT, subselect);
    *size = input_config_read(base, VIRTIO_INPUT_CFG_SIZE);

    if (*size > VIRTIO_INPUT_CFG_DATA_MAX)
    {
        dump("VirtIO input config size invalid", *size);
        return -1;
    }

    for (uint8_t i = 0; i < *size; ++i)
    {
        data[i] = input_config_read(base, VIRTIO_INPUT_CFG_DATA + i);
    }
    return 0;
}

static int input_bitmap_has(const uint8_t *bitmap, uint8_t size, uint16_t code)
{
    uint16_t byte = code / 8;
    return byte < size && (bitmap[byte] & (1U << (code % 8))) != 0;
}

static int input_has_code(uint64_t base, uint8_t event_type, uint16_t code)
{
    uint8_t bitmap[VIRTIO_INPUT_CFG_DATA_MAX];
    uint8_t size;

    if (input_read_config_data(
            base,
            VIRTIO_INPUT_CFG_EV_BITS,
            event_type,
            bitmap,
            &size) < 0)
    {
        return 0;
    }
    return input_bitmap_has(bitmap, size, code);
}

static void input_report_identity(uint64_t base, uint32_t index)
{
    uint8_t name[VIRTIO_INPUT_CFG_DATA_MAX];
    uint8_t name_size;

    dump("VirtIO input device", index);
    if (input_read_config_data(
            base,
            VIRTIO_INPUT_CFG_ID_NAME,
            0,
            name,
            &name_size) < 0)
    {
        return;
    }

    puts("VirtIO input name: ");
    if (name_size == 0)
    {
        puts("(not provided)");
    }
    else
    {
        for (uint8_t i = 0; i < name_size; ++i)
        {
            uint8_t character = name[i];
            putchar(character >= 32 && character < 127 ? character : '?');
        }
    }
    puts("\n");

    int has_key = input_has_code(base, VIRTIO_INPUT_EV_KEY, VIRTIO_INPUT_KEY_A) &&
                  input_has_code(base, VIRTIO_INPUT_EV_KEY, VIRTIO_INPUT_KEY_ENTER);
    int has_relative_pointer =
        input_has_code(base, VIRTIO_INPUT_EV_REL, VIRTIO_INPUT_REL_X) &&
        input_has_code(base, VIRTIO_INPUT_EV_REL, VIRTIO_INPUT_REL_Y) &&
        input_has_code(base, VIRTIO_INPUT_EV_KEY, VIRTIO_INPUT_BTN_LEFT);
    int has_absolute_pointer =
        input_has_code(base, VIRTIO_INPUT_EV_ABS, VIRTIO_INPUT_ABS_X) &&
        input_has_code(base, VIRTIO_INPUT_EV_ABS, VIRTIO_INPUT_ABS_Y);

    if (has_key)
    {
        puts("VirtIO input capabilities: keyboard keys\n");
    }
    if (has_relative_pointer)
    {
        puts("VirtIO input capabilities: relative pointer\n");
    }
    if (has_absolute_pointer)
    {
        puts("VirtIO input capabilities: absolute pointer\n");
    }
    if (!has_key && !has_relative_pointer && !has_absolute_pointer)
    {
        puts("VirtIO input capabilities: other\n");
    }
}

static void input_mark_failed(uint64_t base)
{
    VIRTIO_TOT_REG(base, 0x070) |= VIRTIO_STATUS_FAILED;
}

static int8_t input_key_to_character(uint16_t code, int shifted, int caps_lock)
{
    static const uint16_t letter_codes[] = {
        30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50,
        49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44};
    static const uint16_t digit_codes[] = {
        2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    static const int8_t normal_digits[] = "1234567890";
    static const int8_t shifted_digits[] = "!@#$%^&*()";

    for (uint32_t i = 0; i < sizeof(letter_codes) / sizeof(letter_codes[0]); ++i)
    {
        if (code == letter_codes[i])
        {
            int8_t letter = (int8_t)('a' + i);
            return (shifted != caps_lock) ? (int8_t)(letter - 'a' + 'A') : letter;
        }
    }

    for (uint32_t i = 0; i < sizeof(digit_codes) / sizeof(digit_codes[0]); ++i)
    {
        if (code == digit_codes[i])
        {
            return shifted ? shifted_digits[i] : normal_digits[i];
        }
    }

    switch (code)
    {
    case VIRTIO_INPUT_KEY_ESC:
        return IO_KEY_ESC;
    case VIRTIO_INPUT_KEY_LEFTCTRL:
    case VIRTIO_INPUT_KEY_RIGHTCTRL:
        return IO_KEY_CTRL;
    case VIRTIO_INPUT_KEY_F1:
        return IO_KEY_F1;
    case VIRTIO_INPUT_KEY_F1 + 1:
        return IO_KEY_F2;
    case VIRTIO_INPUT_KEY_F1 + 2:
        return IO_KEY_F3;
    case VIRTIO_INPUT_KEY_F1 + 3:
        return IO_KEY_F4;
    case VIRTIO_INPUT_KEY_F1 + 4:
        return IO_KEY_F5;
    case VIRTIO_INPUT_KEY_F1 + 5:
        return IO_KEY_F6;
    case VIRTIO_INPUT_KEY_F1 + 6:
        return IO_KEY_F7;
    case VIRTIO_INPUT_KEY_F1 + 7:
        return IO_KEY_F8;
    case VIRTIO_INPUT_KEY_F1 + 8:
        return IO_KEY_F9;
    case VIRTIO_INPUT_KEY_F10:
        return IO_KEY_F10;
    case VIRTIO_INPUT_KEY_F11:
        return IO_KEY_F11;
    case VIRTIO_INPUT_KEY_F12:
        return IO_KEY_F12;
    case VIRTIO_INPUT_KEY_UP:
        return IO_KEY_UP;
    case VIRTIO_INPUT_KEY_DOWN:
        return IO_KEY_DOWN;
    case VIRTIO_INPUT_KEY_LEFT:
        return IO_KEY_LEFT;
    case VIRTIO_INPUT_KEY_RIGHT:
        return IO_KEY_RIGHT;
    case 12:
        return shifted ? '_' : '-';
    case 13:
        return shifted ? '+' : '=';
    case 14:
        return '\b';
    case 15:
        return '\t';
    case 26:
        return shifted ? '{' : '[';
    case 27:
        return shifted ? '}' : ']';
    case 28:
        return '\n';
    case 39:
        return shifted ? ':' : ';';
    case 40:
        return shifted ? '"' : '\'';
    case 41:
        return shifted ? '~' : '`';
    case 43:
        return shifted ? '|' : '\\';
    case 51:
        return shifted ? '<' : ',';
    case 52:
        return shifted ? '>' : '.';
    case 53:
        return shifted ? '?' : '/';
    case 57:
        return ' ';
    default:
        return 0;
    }
}

static void input_queue_character(int8_t character)
{
    uint32_t head = input_char_head;
    uint32_t tail = input_char_tail;

    if (head - tail >= VIRTIO_INPUT_CHAR_QUEUE_SIZE)
    {
        input_char_dropped++;
        return;
    }

    input_char_queue[head % VIRTIO_INPUT_CHAR_QUEUE_SIZE] = character;
    virtio_mb();
    input_char_head = head + 1;
}

static void input_process_key(
    struct virtio_input_device *device,
    const virtio_input_event_t *event)
{
    if (event->type != VIRTIO_INPUT_EV_KEY)
    {
        return;
    }

    if (event->code == VIRTIO_INPUT_KEY_LEFTSHIFT)
    {
        device->left_shift = event->value != 0;
        return;
    }
    if (event->code == VIRTIO_INPUT_KEY_RIGHTSHIFT)
    {
        device->right_shift = event->value != 0;
        return;
    }
    if (event->code == VIRTIO_INPUT_KEY_CAPSLOCK)
    {
        if (event->value == 1)
        {
            device->caps_lock = !device->caps_lock;
        }
        return;
    }
    if (event->value != 1 && event->value != 2)
    {
        return;
    }

    int8_t character = input_key_to_character(
        event->code,
        device->left_shift || device->right_shift,
        device->caps_lock);
    if (character != 0)
    {
        input_queue_character(character);
    }
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
        input_report_identity(base, i);

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

        if (g_virtio_input_irqs[i] >= 32 && g_virtio_input_irqs[i] < 1020)
        {

            gic_set_target_cpu0(g_virtio_input_irqs[i]);
            gic_set_priority(g_virtio_input_irqs[i], 0x80);

            gic_enable_irq(g_virtio_input_irqs[i]);
        }
        else if (g_virtio_input_irqs[i] != 0)
        {
            puts("VirtIO input IRQ is outside the supported GIC SPI range\n");
        }
        else
        {
            puts("VirtIO input IRQ missing; asynchronous input unavailable\n");
        }

        virtio_mmio_notify_queue(base, VIRTIO_INPUT_EVENT_QUEUE);
        dump("VirtIO input ready", i);
    }
}

int vhid_getchar(int8_t *character)
{
    if (character == NULL)
    {
        puts("VirtIO input character output is null\n");
        return -1;
    }

    uint32_t dropped = input_char_dropped;
    if (dropped != input_char_dropped_reported)
    {
        input_char_dropped_reported = dropped;
        dump("VirtIO input characters dropped", dropped);
    }

    uint32_t tail = input_char_tail;
    if (tail == input_char_head)
    {
        return 0;
    }

    virtio_mb();
    *character = input_char_queue[tail % VIRTIO_INPUT_CHAR_QUEUE_SIZE];
    input_char_tail = tail + 1;
    return 1;
}

int vhid_irq_handle(uint32_t irq)
{
    for (uint32_t i = 0; i < g_virtio_input_count; ++i)
    {
        if (g_virtio_input_irqs[i] == 0 || g_virtio_input_irqs[i] != irq)
        {
            continue;
        }

        uint64_t base = g_virtio_input_bases[i];
        virtio_queue_state *queue =
            &input_devices[i].queues[VIRTIO_INPUT_EVENT_QUEUE];
        uint32_t interrupt_status;

        do
        {
            virtio_input_event_t event;
            uint32_t device_index;
            int result;
            do
            {
                result = vhid_get_event(&event, &device_index);
                if (result > 0 && device_index < g_virtio_input_count)
                {
                    input_process_key(&input_devices[device_index], &event);
                }
            } while (result > 0);

            interrupt_status = VIRTIO_TOT_REG(base, 0x060);

            if (interrupt_status != 0)
            {
                VIRTIO_TOT_REG(base, 0x064) = interrupt_status;
            }
            virtio_mb();
        } while (queue->used->idx != input_devices[i].last_used_idx);

        return 1;
    }

    return 0;
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