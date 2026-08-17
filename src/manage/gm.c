#include "manage/_gm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "tools/_virtio.h"
#include "tools/_font.h"

void draw_pixel(
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

    if (gpu_transfer_to_host_2d(
            0,
            0,
            gpu_display_width,
            gpu_display_height) < 0)
    {
        puts("GPU transfer failed\n");
        return;
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

    if (x > gpu_display_width || y > gpu_display_height)
    {
        puts("out of frame!");
        return;
    }

    // 그리기
    for (uint32_t row = 0; row < height; ++row)
    {
        for (uint32_t col = 0; col < width; ++col)
        {
            gpu_framebuffer[(y + row) * gpu_display_width + (x + col)] = color;
        }
    }

    if (gpu_transfer_to_host_2d(
            x,
            y,
            width,
            height) < 0)
    {
        puts("GPU partial transfer failed\n");
        return;
    }

    if (gpu_resource_flush(
            x,
            y,
            width,
            height) < 0)
    {
        puts("GPU partial flush failed\n");
        return;
    }
}