#include "tools/_device.h"

/*
    장치들을 적어두기
*/

#pragma uart

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

#pragma NIC

dcb_t nic_device = {
    .name = "vritQ0",
    .base_addr = VIRTIO_MMIO_BASE,
    .init = nic_dev_init,
    .read = NULL,
    .write = NULL,
    .handler = NULL};

void nic_dev_init(void)
{
    volatile uint32_t host_features;

    // 1. 장치 리셋
    VIRTIO_STATUS = 0;

    // 2. ACKNOWLEDGE + DRIVER
    VIRTIO_STATUS = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;

    // 3. 피처 협상
    host_features = VIRTIO_HOST_FEATURES;
    VIRTIO_GUEST_FEATURES = host_features & 0x00000001U;

    // 4. FEATURES_OK 설정
    VIRTIO_STATUS |= VIRTIO_STATUS_FEATURES_OK;

    // (선택 사항) 장치가 FEATURES_OK를 잘 먹었는지 확인
    if (!(VIRTIO_STATUS & VIRTIO_STATUS_FEATURES_OK))
    {
        puts("Error: Virtio device rejected features!\n");
        return;
    }

    // 💡 주의: 여기서는 아직 DRIVER_OK를 켜지 않습니다!
    // 큐 주소 세팅이 끝난 뒤에 켤 겁니다.
    puts("NIC Features Negotiated Successfully!\n");
}

#pragma endregion