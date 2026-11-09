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

#if DEBUG_DISPLAY_FLAG == 1
static void wm_console_scroll(void)
{
    uint32_t text_height = (gpu_display_height / 8) * 8;
    if (text_height < 8)
    {
        return;
    }

    for (uint32_t y = 8; y < text_height; ++y)
    {
        for (uint32_t x = 0; x < gpu_display_width; ++x)
        {
            gpu_framebuffer[(y - 8) * gpu_display_width + x] =
                gpu_framebuffer[y * gpu_display_width + x];
        }
    }

    uint32_t last_line = text_height - 8;
    for (uint32_t y = last_line; y < text_height; ++y)
    {
        for (uint32_t x = 0; x < gpu_display_width; ++x)
        {
            gpu_framebuffer[y * gpu_display_width + x] = 0xFF000000;
        }
    }

    if (gpu_resource_flush(0, 0, gpu_display_width, text_height) < 0)
    {
        puts("GPU console scroll failed\n");
    }
}

static void wm_console_next_line(void)
{
    wm_console_xpixel = 0;
    ++wm_console_line;

    uint32_t line_count = gpu_display_height / 8;
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
    ltrs_color(wm_console_xpixel, 8 * wm_console_line, run, color);
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

        if (wm_console_xpixel + 8 > gpu_display_width)
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
    if (wm_console_xpixel == 0)
    {
        if (wm_console_line == 0)
        {
            return;
        }
        --wm_console_line;
        wm_console_xpixel = (gpu_display_width / 8) * 8;
    }

    if (wm_console_xpixel < 8)
    {
        return;
    }

    wm_console_xpixel -= 8;
    gm_rect(
        wm_console_xpixel,
        8 * wm_console_line,
        8,
        8,
        0xFF000000);
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
}

// 새로운 창 생성
wcb_t wm_create(
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

    new_wcb.draw = func;

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