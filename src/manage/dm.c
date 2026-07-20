#include "_dm.h"
#include "_io.h"

// 장치 등록 함수
dcb_t *dm_creat(DMv1_driver *driv, uint32_t irq_nr, dcb_t dev)
{
    if (irq_nr >= MAX_DEVI_NUM)
    {
        return NULL;
    }

    // 이미 등록되어 있는지 체크 (중복 등록 방지)
    if (driv->DMv1_mem[irq_nr].name != NULL)
    {
        return NULL;
    }

    // 공간을 주고
    dcb_t *new_device = &(driv->DMv1_mem[irq_nr]);

    // 값 할당
    new_device->irq_nr = irq_nr;

    // 초기화 함수가 제공되었다면 즉시 실행
    if (new_device->init != NULL)
    {
        if (new_device->init() != 0)
        {
            return NULL;
        }
    }

    return new_device;
}

// uart 장치 등록
dcb_t uart_device = {
    .name = "uart0",
    .base_addr = UART0_BASE,
    .init = uart_dev_init,
    .read = uart_dev_read,
    .write = uart_dev_write,
    .handler = NULL // 인터럽트 용 헨들러 아직 미정
};

#pragma region uart

int uart_dev_write(void *buf)
{
    char *str = (char *)buf;
    while (*str)
    {
        // 기존 네 로직 (한 글자씩 쏘기)
        while (*UART0_FR & (1 << 5))
        {
        }
        *UART0_DR = *str++;
    }
    return 0; // 성공
}

int uart_dev_read(void *buf)
{
    char *ptr = (char *)buf;
    while (*UART0_FR & (1 << 4))
    {
    }
    *ptr = *UART0_DR;
    return 1; // 1바이트 읽음
}

// 이거 왜 있지?
void uart_dev_init()
{
    *UART0_CR &= ~(1 << 0);
    // 등등의 기타 초기화
    *UART0_CR |= (1 << 0) | (1 << 8) | (1 << 9);
}

#pragma endregion

void dm_init(void)
{
}
uint8_t timer_init(void)
{
    // 타이머 초기화
    asm volatile("msr cntp_tval_el0, %0" : : "r"(TIMER_TICK));
    return 0;
}

dcb_t *dm_find(DMv1_driver *driv, const char *name)
{
    for (int i = 0; i < MAX_DEVI_NUM; i++)
    {
        if (driv->DMv1_mem[i].name != NULL)
        {
            // 각 디바이스 구조체의 순서를 순회하면서 탐색
            if (strcmp(driv->DMv1_mem[i].name, name) == 0)
            {
                return &(driv->DMv1_mem[i]);
            }
        }
    }
    return NULL; // 못 찾음
}