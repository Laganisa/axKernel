#include "tools/_device.h"

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
    .handler = NULL // 인터럽트 용 헨들러 아직 미정
};

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

    // 1. 장치 리셋
    VIRTIO_STATUS = 0;

    // 2. ACKNOWLEDGE + DRIVER
    VIRTIO_STATUS = VIRTIO_STATUS_ACKNOWLEDGE;
    VIRTIO_STATUS |= VIRTIO_STATUS_DRIVER;

    // Version 1 is the legacy MMIO interface: features are single 32-bit fields.
    host_features = VIRTIO_HOST_FEATURES;

    puts("Host features: ");
    put_hex(host_features);
    puts("\n");

    // Request no optional features until the individual virtio-net features
    // are implemented by the driver.
    VIRTIO_GUEST_FEATURES = 0;

    VIRTIO_GUEST_PAGE_SIZE = 4096;

    puts("NIC Features Negotiated Successfully!\n");
}

#pragma endregion