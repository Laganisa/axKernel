#include "tools/_device.h"
#include "global/_debug.h"

/*
    장치들을 적어두기
*/

#pragma region uart

// uart 장치 등록
dcb_t uart_device = {
    .name = "uart0",
    .base_addr = UART0_BASE,
    .init = uart_dev_init,
    .read = uart_dev_read,
    .write = uart_dev_write,
    .handler = NULL // ? 인터럽트 용 헨들러 아직 미정
};

int uart_dev_write(const void *buf, int len)
{
    const char *str = (const char *)buf;
    for (int i = 0; i < len; i++)
    {
        while (*UART0_FR & (1 << 5))
        {
        }
        *UART0_DR = str[i];
    }
    return len;
}

int uart_dev_read(void *buf)
{
    char *ptr = (char *)buf;
    while (*UART0_FR & (1 << 4))
    {
    }
    *ptr = *UART0_DR;
    return 1;
}

// ? 이거 왜 있지?
void uart_dev_init()
{
    *UART0_CR &= ~(1 << 0);
    *UART0_CR |= (1 << 0) | (1 << 8) | (1 << 9);
}

#pragma endregion

#pragma region NIC

dcb_t nic_device = {
    .name = "vritQ0",
    .base_addr = VIRTIO_MMIO_BASE,
    .init = nic_dev_init,
    .read = NULL,
    .write = NULL,
    .handler = NULL};

void nic_dev_init(void)
{
    uint32_t host_features;

    VIRTIO_STATUS = 0;

    VIRTIO_STATUS = VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER;

    host_features = VIRTIO_HOST_FEATURES;

    // dump("host_features", host_features);

    VIRTIO_GUEST_FEATURES = 0;

    VIRTIO_GUEST_PAGE_SIZE = 4096;

    // log("NIC Features Negotiated Successfully!\n");
}

#pragma endregion
