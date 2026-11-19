#include "manage/_gm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "global/_alloc.h"
#include "manage/_wm.h"
#include "tools/_virtio.h"
#include "tools/_font.h"

static uint32_t *gm_clipboard;
static uint32_t gm_clipboard_width;
static uint32_t gm_clipboard_height;

void gm_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
{
    if (x >= gpu_display_width ||
        y >= gpu_display_height)
    {
        return;
    }
    if (clip_x >= gpu_display_width ||
        clip_y >= gpu_display_height ||
        x < clip_x ||
        y < clip_y ||
        x - clip_x >= clip_width ||
        y - clip_y >= clip_height)
    {
        return;
    }

    gpu_framebuffer[y * gpu_display_width + x] = color;
}

static int gm_clip_rect(
    uint32_t *x,
    uint32_t *y,
    uint32_t *width,
    uint32_t *height,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
{
    if (clip_x >= gpu_display_width ||
        clip_y >= gpu_display_height ||
        *x >= gpu_display_width ||
        *y >= gpu_display_height ||
        *width == 0 ||
        *height == 0)
    {
        return 0;
    }

    if (*width > gpu_display_width - *x)
    {
        *width = gpu_display_width - *x;
    }
    if (*height > gpu_display_height - *y)
    {
        *height = gpu_display_height - *y;
    }
    if (clip_width > gpu_display_width - clip_x)
    {
        clip_width = gpu_display_width - clip_x;
    }
    if (clip_height > gpu_display_height - clip_y)
    {
        clip_height = gpu_display_height - clip_y;
    }

    uint32_t right = *x + *width;
    uint32_t bottom = *y + *height;
    uint32_t clip_right = clip_x + clip_width;
    uint32_t clip_bottom = clip_y + clip_height;

    if (*x < clip_x)
    {
        *x = clip_x;
    }
    if (*y < clip_y)
    {
        *y = clip_y;
    }
    if (right > clip_right)
    {
        right = clip_right;
    }
    if (bottom > clip_bottom)
    {
        bottom = clip_bottom;
    }
    if (right <= *x || bottom <= *y)
    {
        return 0;
    }

    *width = right - *x;
    *height = bottom - *y;
    return 1;
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
    uint32_t color,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
{
    if (!gm_clip_rect(
            &x,
            &y,
            &width,
            &height,
            clip_x,
            clip_y,
            clip_width,
            clip_height))
    {
        return;
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
    uint32_t color,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
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
        gm_pixel(
            (uint32_t)x,
            (uint32_t)y,
            color,
            clip_x,
            clip_y,
            clip_width,
            clip_height);

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

    if (!gm_clip_rect(
            &left,
            &top,
            &width,
            &height,
            clip_x,
            clip_y,
            clip_width,
            clip_height))
    {
        return;
    }

    if (gpu_resource_flush(left, top, width, height) < 0)
    {
        puts("GPU line flush failed\n");
    }
}

void deci(uint32_t x, uint32_t y, uint64_t n)
{
    char digits[21];
    uint8_t length = 0;

    do
    {
        digits[length++] = (char)('0' + n % 10);
        n /= 10;
    } while (n != 0);

    for (uint8_t i = 0; i < length / 2; ++i)
    {
        char digit = digits[i];
        digits[i] = digits[length - i - 1];
        digits[length - i - 1] = digit;
    }

    digits[length] = '\0';
    ltrs(x, y, digits);
}

static void gm_console_get_clip(
    uint32_t *x,
    uint32_t *y,
    uint32_t *width,
    uint32_t *height)
{
    for (uint8_t i = 0; i < wm_slot->cnt; ++i)
    {
        wcb_t *window = &wm_slot->wm_arr[i];
        if (window->is_on && window->pcb_id == wm_slot->now_pcb_id)
        {
            *x = window->x;
            *y = window->y;
            if (*x >= gpu_display_width || *y >= gpu_display_height)
            {
                *width = 0;
                *height = 0;
            }
            else
            {
                *width = window->w;
                *height = window->h;
                if (*width > gpu_display_width - *x)
                {
                    *width = gpu_display_width - *x;
                }
                if (*height > gpu_display_height - *y)
                {
                    *height = gpu_display_height - *y;
                }
            }
            return;
        }
    }

    *x = 0;
    *y = 0;
    *width = gpu_display_width;
    *height = gpu_display_height;
}

static void gm_console_next_line(
    uint32_t *cursor_x,
    uint32_t *cursor_y,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
{
    uint32_t text_width = (clip_width / 8) * 8;
    uint32_t text_height = (clip_height / 8) * 8;
    if (text_width < 8 || text_height < 8)
    {
        return;
    }

    *cursor_x = clip_x;
    uint32_t bottom = clip_y + text_height;
    if (text_height > 8 && *cursor_y <= bottom - 16)
    {
        *cursor_y += 8;
        return;
    }

    if (text_height > 8)
    {
        for (uint32_t row = 8; row < text_height; ++row)
        {
            for (uint32_t col = 0; col < text_width; ++col)
            {
                gpu_framebuffer[(clip_y + row - 8) * gpu_display_width + clip_x + col] =
                    gpu_framebuffer[(clip_y + row) * gpu_display_width + clip_x + col];
            }
        }
    }

    for (uint32_t row = text_height - 8; row < text_height; ++row)
    {
        for (uint32_t col = 0; col < text_width; ++col)
        {
            gpu_framebuffer[(clip_y + row) * gpu_display_width + clip_x + col] =
                0xFF000000;
        }
    }

    if (gpu_resource_flush(clip_x, clip_y, text_width, text_height) < 0)
    {
        puts("GPU input console scroll failed\n");
    }
    *cursor_y = bottom - 8;
}

static void gm_console_erase_char(
    uint32_t *cursor_x,
    uint32_t *cursor_y,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
{
    if (*cursor_x <= clip_x)
    {
        if (*cursor_y <= clip_y)
        {
            return;
        }
        *cursor_y -= 8;
        *cursor_x = clip_x + (clip_width / 8) * 8;
    }

    if (*cursor_x < clip_x + 8)
    {
        return;
    }

    *cursor_x -= 8;
    gm_rect(
        *cursor_x,
        *cursor_y,
        8,
        8,
        0xFF000000,
        clip_x,
        clip_y,
        clip_width,
        clip_height);
}

static void gm_console_echo_char(
    char c,
    uint32_t *cursor_x,
    uint32_t *cursor_y,
    uint32_t clip_x,
    uint32_t clip_y,
    uint32_t clip_width,
    uint32_t clip_height)
{
    if (c == '\n')
    {
        gm_console_next_line(
            cursor_x, cursor_y, clip_x, clip_y, clip_width, clip_height);
        return;
    }

    if (clip_width < 8 || clip_height < 8)
    {
        return;
    }
    uint32_t text_width = (clip_width / 8) * 8;
    if (*cursor_x + 8 > clip_x + text_width)
    {
        gm_console_next_line(
            cursor_x, cursor_y, clip_x, clip_y, clip_width, clip_height);
    }

    char character[2] = {c, '\0'};
    ltrs_color_clip(
        *cursor_x,
        *cursor_y,
        character,
        0xFFFFFFFF,
        clip_x,
        clip_y,
        clip_width,
        clip_height);
    *cursor_x += 8;
}

int32_t gtb(uint32_t x, uint32_t y, char *s, int32_t max_len)
{
    if (s == NULL || max_len <= 0)
    {
        puts("Window input buffer is invalid\n");
        return -1;
    }

    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
    gm_console_get_clip(&clip_x, &clip_y, &clip_width, &clip_height);
    if (clip_width < 8 ||
        clip_height < 8 ||
        x < clip_x ||
        x - clip_x >= clip_width ||
        y < clip_y ||
        y - clip_y > clip_height - 8)
    {
        s[0] = '\0';
        puts("Window input position is outside the active window\n");
        return -1;
    }

    uint32_t cursor_x = x;
    uint32_t cursor_y = y;
    int32_t i = 0;
    while (i < max_len - 1)
    {
        int8_t c;
        int result = vhid_getchar(&c);

        if (result < 0)
        {
            s[i] = '\0';
            puts("Window input failed\n");
            return -1;
        }
        if (result == 0)
        {
            uint64_t daif;
            __asm__ volatile("mrs %0, daif" : "=r"(daif));
            if ((daif & (1ULL << 7)) == 0)
            {
                __asm__ volatile("wfi");
            }
            continue;
        }

        if (c == '\r' || c == '\n')
        {
            break;
        }
        if (c == '\b' || c == 127)
        {
            if (i > 0)
            {
                --i;
                gm_console_erase_char(
                    &cursor_x, &cursor_y,
                    clip_x, clip_y, clip_width, clip_height);
            }
            continue;
        }
        if (c >= 32 && c < 127)
        {
            s[i++] = (char)c;
            gm_console_echo_char(
                (char)c, &cursor_x, &cursor_y,
                clip_x, clip_y, clip_width, clip_height);
        }
    }

    s[i] = '\0';
    return i;
}