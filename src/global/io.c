#include "manage/_fm.h"
#include "global/_io.h"
#include "_macro.h"
#include "tools/_virtio.h"
#include "tools/_font.h"

uint32_t io_console_line;
uint32_t io_console_xpixel;

#if DEBUG_DISPLAY_FLAG == 1
#include "manage/_gm.h"

static void io_console_scroll(void)
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

static void io_console_next_line(void)
{
    io_console_xpixel = 0;
    ++io_console_line;

    uint32_t line_count = gpu_display_height / 8;
    if (line_count != 0 && io_console_line >= line_count)
    {
        io_console_scroll();
        io_console_line = line_count - 1;
    }
}

static void io_console_flush_run(
    char *run,
    uint32_t *run_len,
    uint32_t color)
{
    if (*run_len == 0)
    {
        return;
    }

    run[*run_len] = '\0';
    ltrs_color(io_console_xpixel, 8 * io_console_line, run, color);
    io_console_xpixel += *run_len * 8;
    *run_len = 0;
}

static void io_console_erase_char(void)
{
    if (io_console_xpixel == 0)
    {
        if (io_console_line == 0)
        {
            return;
        }
        --io_console_line;
        io_console_xpixel = (gpu_display_width / 8) * 8;
    }

    if (io_console_xpixel < 8)
    {
        return;
    }

    io_console_xpixel -= 8;
    gm_rect(
        io_console_xpixel,
        8 * io_console_line,
        8,
        8,
        0xFF000000);
}
#endif

/*
    커널용 입출력 함수들
*/

// UART 초기화
void uart_init(void)
{
    // QEMU virt는 이미 UART 활성화됨
    // 아무것도 안 해도 됨
}

void putchar(int8_t c)
{
    // UART_FR 5번 비트: TXFF (보낼 데이터 칸이 꽉 찼는지 확인)
    while (*UART0_FR & (1 << 5))
    {
    }

    *UART0_DR = c;
}

// 문자열 출력
void puts(const int8_t *s)
{
    while (*s)
    {
        putchar(*s++);
    }
}

// 문자 입력 (UART에서 수신 대기)
int8_t getchar(void)
{
    while (1)
    {
        int8_t character;
        int result = vhid_getchar(&character);

        if (result > 0)
        {
            return character;
        }
        if (!(*UART0_FR & (1 << 4)))
        {
            return (int8_t)*UART0_DR;
        }

        uint64_t daif;
        __asm__ volatile("mrs %0, daif" : "=r"(daif));
        if ((daif & (1ULL << 7)) == 0)
        {
            __asm__ volatile("wfi");
        }
    }
}

// 문자열 입력 (Enter까지 받음)
void gets(int8_t *s, int32_t max_len)
{
    if (s == NULL || max_len <= 0)
    {
        return;
    }

    int32_t i = 0;
    while (i < max_len - 1)
    {
        int8_t c = getchar();

        // 1. 엔터 처리
        if (c == '\r' || c == '\n')
        {
#if DEBUG_DISPLAY_FLAG == 0
            putchar('\n');
#else
            io_puts_color("\n", 0xFFFFFFFF);
#endif
            break;
        }
        // 2. 백스페이스 처리 (ASCII 8: \b, 127: DEL)
        else if ((c == '\b' || c == 127))
        {
            if (i > 0)
            {
                i--;
                s[i] = '\0'; // 버퍼에서 제거
#if DEBUG_DISPLAY_FLAG == 0
                puts("\b \b"); // 화면에서 제거
#else
                io_console_erase_char();
#endif
            }
            // i가 0일 때는 아무것도 안 함 (프롬프트 보호)
        }
        // 3. 일반 문자 입력 (에코는 여기서만!)
        else if (c >= 32 && c < 127)
        {
            s[i++] = c;
#if DEBUG_DISPLAY_FLAG == 0
            putchar(c); // 백스페이스나 엔터가 아닐 때만 화면에 출력
#else
            char character[2] = {(char)c, '\0'};
            io_puts_color(character, 0xFFFFFFFF);
#endif
        }
    }
    s[i] = '\0';
}

uint16_t io_puts_color(const char *s, uint32_t color)
{
    if (s == NULL)
    {
        puts("Console output string is null\n");
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
            io_console_flush_run(run, &run_len, color);
            io_console_next_line();
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

        if (io_console_xpixel + 8 > gpu_display_width)
        {
            io_console_flush_run(run, &run_len, color);
            io_console_next_line();
        }

        run[run_len++] = *s++;
        if (run_len == sizeof(run) - 1)
        {
            io_console_flush_run(run, &run_len, color);
        }
    }

    io_console_flush_run(run, &run_len, color);
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

// 부호 없는 64비트를 십진수로
void put_uint(uint64_t n)
{
    int8_t buf[21];
    int32_t i = 19;
    buf[20] = '\0';

    if (n == 0)
        buf[i--] = '0';

    while (n > 0 && i >= 0)
    {
        buf[i--] = (n % 10) + '0';
        n /= 10;
    }

    puts(&buf[i + 1]);
}

void put_hex2(uint64_t d)
{
    char *hex = "0123456789ABCDEF";
    uint8_t val = d & 0xFF;         // 하위 8비트(2자리)만 추출
    putchar(hex[(val >> 4) & 0xF]); // 16의 자리 (첫 번째 자릿수)
    putchar(hex[val & 0xF]);        // 1의 자리 (두 번째 자릿수)
}

// 16진수 출력 함수
void put_hex(uint64_t d)
{
    char *hex = "0123456789ABCDEF";
    puts("0x");
    for (int i = 60; i >= 0; i -= 4)
    {
        putchar(hex[(d >> i) & 0xF]);
    }
    puts("\n");
}

void uart_flush(void)
{
    // UART_FR 레지스터 4번 비트(RX FIFO empty)가 0(데이터 있음)인 동안 계속 읽음
    while (!(*UART0_FR & (1 << 4)))
    {
        (void)*UART0_DR; // 데이터 읽어서 버림
    }
}

// 문자 비교 함수
int32_t strcmp(const int8_t *s1, const int8_t *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(uint8_t *)s1 - *(uint8_t *)s2;
}

void *memset(void *ptr, int value, size_t num)
{
    unsigned char *p = (unsigned char *)ptr;

    for (size_t i = 0; i < num; i++)
    {
        p[i] = (unsigned char)value;
    }

    return ptr;
}

char *strstr(const char *s, const char *sub)
{
    if (!*sub)
        return (char *)s;
    for (; *s; s++)
    {
        const char *h = s;
        const char *n = sub;
        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }
        if (!*n)
            return (char *)s;
    }
    return 0;
}

uint64_t str_to_uint(const char *str)
{
    uint64_t val = 0;
    while (*str >= '0' && *str <= '9')
    {
        val = val * 10 + (*str - '0');
        str++;
    }
    return val;
}
