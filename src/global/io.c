// #include "types.h"
// #include "defs.h"

// #include "sync.h"
#include "_fm.h"

// #include "pm.h"
// #include "sect.h"

// UART 초기화
void uart_init(void)
{
    // QEMU virt는 이미 UART 활성화됨
    // 아무것도 안 해도 됨
}

#pragma region string

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
    // UART_FR 레지스터 4번 비트: RX FIFO empty
    // 데이터가 올 때까지 대기
    while (*UART0_FR & (1 << 4))
    {
        // UART가 데이터를 기다리는 중
    }
    return *UART0_DR;
}

// 문자열 입력 (Enter까지 받음)
void gets(int8_t *s, int32_t max_len)
{
    int32_t i = 0;
    while (i < max_len - 1)
    {
        int8_t c = getchar();

        // 1. 엔터 처리
        if (c == '\r' || c == '\n')
        {
            putchar('\n');
            break;
        }
        // 2. 백스페이스 처리 (ASCII 8: \b, 127: DEL)
        else if ((c == '\b' || c == 127))
        {
            if (i > 0)
            {
                i--;
                s[i] = '\0';   // 버퍼에서 제거
                puts("\b \b"); // 화면에서 제거
            }
            // i가 0일 때는 아무것도 안 함 (프롬프트 보호)
        }
        // 3. 일반 문자 입력 (에코는 여기서만!)
        else if (c >= 32 && c < 127)
        {
            s[i++] = c;
            putchar(c); // 백스페이스나 엔터가 아닐 때만 화면에 출력
        }
    }
    s[i] = '\0';
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

// 제거 가능한 입력
void remo_get(int8_t *s, int32_t max_len)
{
    int32_t i = 0;
    while (i < max_len - 1)
    {
        int8_t c = getchar();

        // 1. 엔터: 입력 완료 시점
        if (c == '\r' || c == '\n')
        {
            putchar('\n');
            break;
        }

        // 2. 백스페이스(0x08) 및 DEL(0x7F): 제거(Remove) 로직
        else if (c == '\b' || c == 0x7F)
        {
            if (i > 0) // 프롬프트 가드 (방화벽 역할)
            {
                i--;           // 인덱스 뒤로
                s[i] = '\0';   // 버퍼에서 제거
                puts("\b \b"); // 화면에서 한 글자 지우기
            }
        }

        // 3. 일반 문자: 버퍼 추가 및 에코(Echo)
        else if (c >= 32 && c < 127)
        {
            s[i++] = c; // 버퍼에 저장
            putchar(c); // 화면에 출력
        }
    }
    s[i] = '\0'; // 최종 문자열 마무리
}

#pragma endregion

// PCB-related 전역 변수
// ! 이거 왜 있음 ??????
void *current_pcb_addr = 0;