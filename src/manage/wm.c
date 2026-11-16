#include "manage/_gm.h"
#include "manage/_wm.h"
#include "global/_debug.h"
#include "global/_io.h"
#include "tools/_virtio.h"
#include "tools/_font.h"
#include "handler/_irq.h"

extern pcb_t *get_current_proc_addr(void);

#pragma region 화면 내부

uint32_t wm_console_line;
uint32_t wm_console_xpixel;

static void wm_get_clip(
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

#if DEBUG_DISPLAY_FLAG == 1
static void wm_console_scroll(void)
{
    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
    wm_get_clip(&clip_x, &clip_y, &clip_width, &clip_height);

    uint32_t text_width = (clip_width / 8) * 8;
    uint32_t text_height = (clip_height / 8) * 8;
    if (text_width < 8 || text_height < 8)
    {
        return;
    }

    for (uint32_t y = 8; y < text_height; ++y)
    {
        for (uint32_t x = 0; x < text_width; ++x)
        {
            gpu_framebuffer[(clip_y + y - 8) * gpu_display_width + clip_x + x] =
                gpu_framebuffer[(clip_y + y) * gpu_display_width + clip_x + x];
        }
    }

    uint32_t last_line = text_height - 8;
    for (uint32_t y = last_line; y < text_height; ++y)
    {
        for (uint32_t x = 0; x < text_width; ++x)
        {
            gpu_framebuffer[(clip_y + y) * gpu_display_width + clip_x + x] =
                0xFF000000;
        }
    }

    if (gpu_resource_flush(clip_x, clip_y, text_width, text_height) < 0)
    {
        puts("GPU console scroll failed\n");
    }
}

static void wm_console_next_line(void)
{
    wm_console_xpixel = 0;
    ++wm_console_line;

    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
    wm_get_clip(&clip_x, &clip_y, &clip_width, &clip_height);
    uint32_t line_count = clip_height / 8;
    if (line_count != 0 && wm_console_line >= line_count)
    {
        wm_console_scroll();
        wm_console_line = line_count - 1;
    }
}

static void wm_console_flush_run(
    char *run,
    uint32_t *run_len,
    uint32_t color)
{
    if (*run_len == 0)
    {
        return;
    }

    run[*run_len] = '\0';
    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
    wm_get_clip(&clip_x, &clip_y, &clip_width, &clip_height);
    ltrs_color_clip(
        clip_x + wm_console_xpixel,
        clip_y + 8 * wm_console_line,
        run,
        color,
        clip_x,
        clip_y,
        clip_width,
        clip_height);
    wm_console_xpixel += *run_len * 8;
    *run_len = 0;
}
#endif

static uint16_t wm_console_puts_color(const char *s, uint32_t color);

static void wm_compact_strings(void)
{
    uint32_t offset = 0;
    for (uint8_t i = 0; i < wm_slot->cmd_cnt; ++i)
    {
        gui_cmd_t *cmd = &wm_slot->cmd_arr[i];
        uint16_t len = cmd->len;
        const char *source = cmd->str;

        for (uint32_t j = 0; j <= len; ++j)
        {
            wm_slot->str_buf[offset + j] = source[j];
        }
        cmd->str = &wm_slot->str_buf[offset];
        offset += (uint32_t)len + 1;
    }
}

static uint16_t wm_queue_puts(const char *s, uint32_t color, uint8_t pcb_id)
{
    wm_compact_strings();

    uint32_t string_used = 0;
    for (uint8_t i = 0; i < wm_slot->cmd_cnt; ++i)
    {
        string_used += (uint32_t)wm_slot->cmd_arr[i].len + 1;
    }

    uint32_t available = sizeof(wm_slot->str_buf) - string_used;
    uint32_t len = 0;
    while (len < available && s[len] != '\0')
    {
        ++len;
    }
    if (len == available || wm_slot->cmd_cnt >= WM_CMD_QUEUE_SIZE)
    {
        return 0;
    }

    gui_cmd_t *cmd = &wm_slot->cmd_arr[wm_slot->cmd_cnt];
    cmd->type = WM_CMD_PUTS;
    cmd->pcb_id = pcb_id;
    cmd->color = color;
    cmd->len = (uint16_t)len;
    cmd->str = &wm_slot->str_buf[string_used];

    for (uint32_t i = 0; i <= len; ++i)
    {
        wm_slot->str_buf[string_used + i] = s[i];
    }
    ++wm_slot->cmd_cnt;
    return 1;
}

static uint16_t wm_puts_color(const char *s, uint32_t color)
{
    if (s == NULL)
    {
        puts("Window output string is null\n");
        return 0;
    }

    pcb_t *now_pcb = get_current_proc_addr();

    uint8_t wm_id = wm_slot->now_pcb_id;

    // 활성 창이라면 그리기
    if (now_pcb == NULL || now_pcb->id == wm_id)
    {
        return wm_console_puts_color(s, color);
    }

    log("a");
    // 활성창이 아니라면 큐에 넣기
    // 명령 큐가 있다 가정

    // 만약 큐 크기가 다 차면 0을 리턴

    // 문자열을 wm_slot str buf에 복사하고 그 주소를 저장하기

    // 명령 큐에 넣기

    return wm_queue_puts(s, color, now_pcb->id);
}

uint16_t wputs(const char *s)
{
    return wm_puts_color(s, 0xFFFFFFFF);
}

uint16_t wputs_color(const char *s, uint32_t color)
{
    return wm_puts_color(s, color);
}

static uint16_t wm_console_puts_color(const char *s, uint32_t color)
{
    if (s == NULL)
    {
        puts("Window output string is null\n");
        return 0;
    }

#if DEBUG_DISPLAY_FLAG == 1
    if (gpu_display_width < 8 || gpu_display_height < 8)
    {
        puts("GPU console display is too small\n");
        return 0;
    }

    char run[128];
    uint32_t run_len = 0;
    uint16_t newline_count = 0;

    while (*s != '\0')
    {
        if (*s == '\n')
        {
            wm_console_flush_run(run, &run_len, color);
            wm_console_next_line();
            if (newline_count < 0xFFFF)
            {
                ++newline_count;
            }
            ++s;
            continue;
        }

        if (*s == '\r')
        {
            ++s;
            continue;
        }

        uint32_t clip_x;
        uint32_t clip_y;
        uint32_t clip_width;
        uint32_t clip_height;
        wm_get_clip(&clip_x, &clip_y, &clip_width, &clip_height);
        if (wm_console_xpixel + 8 > clip_width)
        {
            wm_console_flush_run(run, &run_len, color);
            wm_console_next_line();
        }

        run[run_len++] = *s++;
        if (run_len == sizeof(run) - 1)
        {
            wm_console_flush_run(run, &run_len, color);
        }
    }

    wm_console_flush_run(run, &run_len, color);
    return newline_count;
#else
    if (color == 0xFFFF0000)
    {
        puts((const int8_t *)"\033[31m");
    }
    puts((const int8_t *)s);
    if (color == 0xFFFF0000)
    {
        puts((const int8_t *)"\033[0m");
    }

    uint16_t newline_count = 0;
    while (*s != '\0')
    {
        if (*s++ == '\n' && newline_count < 0xFFFF)
        {
            ++newline_count;
        }
    }
    return newline_count;
#endif
}

void wconsole_erase_char(void)
{
#if DEBUG_DISPLAY_FLAG == 1
    uint32_t clip_x;
    uint32_t clip_y;
    uint32_t clip_width;
    uint32_t clip_height;
    wm_get_clip(&clip_x, &clip_y, &clip_width, &clip_height);

    if (wm_console_xpixel == 0)
    {
        if (wm_console_line == 0)
        {
            return;
        }
        --wm_console_line;
        wm_console_xpixel = (clip_width / 8) * 8;
    }

    if (wm_console_xpixel < 8)
    {
        return;
    }

    wm_console_xpixel -= 8;
    gm_rect(
        clip_x + wm_console_xpixel,
        clip_y + 8 * wm_console_line,
        8,
        8,
        0xFF000000,
        clip_x,
        clip_y,
        clip_width,
        clip_height);
#endif
}

int32_t wget(char *s, int32_t max_len)
{
    if (s == NULL || max_len <= 0)
    {
        puts("Window input buffer is invalid\n");
        return -1;
    }

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
            wputs("\n");
            break;
        }
        if (c == '\b' || c == 127)
        {
            if (i > 0)
            {
                --i;
                wconsole_erase_char();
            }
            continue;
        }
        if (c >= 32 && c < 127)
        {
            s[i++] = (char)c;
            char character[2] = {(char)c, '\0'};
            wputs(character);
        }
    }

    s[i] = '\0';
    return i;
}

#pragma endregion

#pragma region 화면 외부

// 윈도우 슬롯 초기화
void wm_init()
{
    wm_slot->now_pcb_id = 0;
    wm_slot->cnt = 0;
    wm_slot->cmd_cnt = 0;
    wm_console_line = 0;
    wm_console_xpixel = 0;
}

// 새로운 창 생성
wcb_t wm_create(
    uint8_t pcb_id,
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h,
    void (*func)(uint32_t, uint32_t))
{
    wcb_t new_wcb;

    new_wcb.x = x;
    new_wcb.y = y;
    new_wcb.w = w;
    new_wcb.h = h;

    new_wcb.is_on = 1;
    new_wcb.pcb_id = pcb_id;
    new_wcb.draw = func;

    if (wm_slot->cnt >= sizeof(wm_slot->wm_arr) / sizeof(wm_slot->wm_arr[0]))
    {
        puts("Window slot is full\n");
        new_wcb.is_on = 0;
        return new_wcb;
    }

    wm_slot->wm_arr[wm_slot->cnt++] = new_wcb;
    wm_slot->now_pcb_id = pcb_id;
    wm_console_line = 0;
    wm_console_xpixel = 0;

    return new_wcb;
}

// 창 교체
void wm_switch()
{
    gui_cmd_t pending[WM_CMD_QUEUE_SIZE];
    uint8_t pending_count = 0;
    uint8_t active_pcb_id = wm_slot->now_pcb_id;

    for (uint8_t i = 0; i < wm_slot->cmd_cnt; ++i)
    {
        gui_cmd_t cmd = wm_slot->cmd_arr[i];
        if (cmd.type == WM_CMD_PUTS && cmd.pcb_id == active_pcb_id)
        {
            wm_console_puts_color(cmd.str, cmd.color);
        }
        else
        {
            pending[pending_count++] = cmd;
        }
    }

    wm_slot->cmd_cnt = pending_count;
    uint32_t string_used = 0;
    for (uint8_t i = 0; i < pending_count; ++i)
    {
        uint16_t len = pending[i].len;
        const char *source = pending[i].str;
        for (uint32_t j = 0; j <= len; ++j)
        {
            wm_slot->str_buf[string_used + j] = source[j];
        }
        pending[i].str = &wm_slot->str_buf[string_used];
        wm_slot->cmd_arr[i] = pending[i];
        string_used += (uint32_t)len + 1;
    }
}

// 창 삭제
void wm_delate()
{
}

#pragma endregion