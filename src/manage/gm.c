#include "manage/_gm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "global/_alloc.h"
#include "tools/_virtio.h"
#include "tools/_font.h"

static uint32_t *gm_clipboard;
static uint32_t gm_clipboard_width;
static uint32_t gm_clipboard_height;

void gm_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color)
{
    if (x >= gpu_display_width ||
        y >= gpu_display_height)
    {
        return;
    }

    gpu_framebuffer[y * gpu_display_width + x] = color;
}

void gm_full(uint32_t color)
{
    uint32_t pixel_count =
        gpu_display_width * gpu_display_height;

    for (uint32_t i = 0; i < pixel_count; ++i)
    {
        gpu_framebuffer[i] = color;
    }

    if (gpu_resource_flush(
            0,
            0,
            gpu_display_width,
            gpu_display_height) < 0)
    {
        puts("GPU flush failed\n");
        return;
    }
}

void gm_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color)
{
    if (x >= gpu_display_width ||
        y >= gpu_display_height ||
        width == 0 ||
        height == 0)
    {
        return;
    }

    if (width > gpu_display_width - x)
    {
        width = gpu_display_width - x;
    }
    if (height > gpu_display_height - y)
    {
        height = gpu_display_height - y;
    }

    for (uint32_t row = 0; row < height; ++row)
    {
        for (uint32_t col = 0; col < width; ++col)
        {
            gpu_framebuffer[(y + row) * gpu_display_width + (x + col)] = color;
        }
    }

    if (gpu_resource_flush(
            x,
            y,
            width,
            height) < 0)
    {
        puts("GPU partial flush failed\n");
    }
}

void gm_copy(
    uint32_t x1,
    uint32_t y1,
    uint32_t x2,
    uint32_t y2)
{
    if (x1 > x2 ||
        y1 > y2 ||
        x2 >= gpu_display_width ||
        y2 >= gpu_display_height)
    {
        puts("GPU copy rectangle is out of bounds\n");
        return;
    }

    uint32_t width = x2 - x1 + 1;
    uint32_t height = y2 - y1 + 1;
    uint64_t byte_count =
        (uint64_t)width * height * sizeof(uint32_t);

    if (byte_count >
        (uint64_t)((uint32_t)-1) - sizeof(heap_block_t))
    {
        puts("GPU copy rectangle is too large\n");
        return;
    }

    uint32_t *clipboard =
        (uint32_t *)heap_alloc((uint32_t)byte_count);

    if (clipboard == NULL)
    {
        puts("GPU clipboard allocation failed\n");
        return;
    }

    for (uint32_t row = 0; row < height; ++row)
    {
        for (uint32_t col = 0; col < width; ++col)
        {
            clipboard[row * width + col] =
                gpu_framebuffer[(y1 + row) * gpu_display_width + x1 + col];
        }
    }

    heap_free(gm_clipboard);
    gm_clipboard = clipboard;
    gm_clipboard_width = width;
    gm_clipboard_height = height;
}

void gm_paste(uint32_t x, uint32_t y)
{
    if (gm_clipboard == NULL)
    {
        puts("GPU clipboard is empty\n");
        return;
    }

    if (x >= gpu_display_width || y >= gpu_display_height)
    {
        puts("GPU paste position is out of bounds\n");
        return;
    }

    uint32_t width = gm_clipboard_width;
    uint32_t height = gm_clipboard_height;

    if (width > gpu_display_width - x)
    {
        width = gpu_display_width - x;
    }
    if (height > gpu_display_height - y)
    {
        height = gpu_display_height - y;
    }

    for (uint32_t row = 0; row < height; ++row)
    {
        for (uint32_t col = 0; col < width; ++col)
        {
            gpu_framebuffer[(y + row) * gpu_display_width + x + col] =
                gm_clipboard[row * gm_clipboard_width + col];
        }
    }

    if (gpu_resource_flush(x, y, width, height) < 0)
    {
        puts("GPU paste flush failed\n");
    }
}

void gm_line(
    uint32_t x1,
    uint32_t y1,
    uint32_t x2,
    uint32_t y2,
    uint32_t color)
{
    if (x1 >= gpu_display_width ||
        x2 >= gpu_display_width ||
        y1 >= gpu_display_height ||
        y2 >= gpu_display_height)
    {
        puts("GPU line endpoint is out of bounds\n");
        return;
    }

    int64_t x = x1;
    int64_t y = y1;
    int64_t end_x = x2;
    int64_t end_y = y2;
    int64_t dx = end_x >= x ? end_x - x : x - end_x;
    int64_t dy = end_y >= y ? y - end_y : end_y - y;
    int64_t step_x = x < end_x ? 1 : -1;
    int64_t step_y = y < end_y ? 1 : -1;
    int64_t error = dx + dy;

    while (1)
    {
        gm_pixel((uint32_t)x, (uint32_t)y, color);

        if (x == end_x && y == end_y)
        {
            break;
        }

        int64_t doubled_error = 2 * error;
        if (doubled_error >= dy)
        {
            error += dy;
            x += step_x;
        }
        if (doubled_error <= dx)
        {
            error += dx;
            y += step_y;
        }
    }

    uint32_t left = x1 < x2 ? x1 : x2;
    uint32_t top = y1 < y2 ? y1 : y2;
    uint32_t width = (x1 < x2 ? x2 - x1 : x1 - x2) + 1;
    uint32_t height = (y1 < y2 ? y2 - y1 : y1 - y2) + 1;

    if (gpu_resource_flush(left, top, width, height) < 0)
    {
        puts("GPU line flush failed\n");
    }
}