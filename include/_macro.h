#ifndef __KERNEL_MACRO_H__
#define __KERNEL_MACRO_H__

/*
    장치들의 주소를 적어두는 헤더
*/

#include "_types.h"

#pragma region UART

// QEMU virt 머신의 PL011 UART 주소
#define UART0_BASE 0x09000000
#define UART0_DR ((volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_FR ((volatile uint32_t *)(UART0_BASE + 0x18))
#define UART0_CR ((volatile uint32_t *)(UART0_BASE + 0x30))

#pragma endregion

#pragma region GIC
// QEMU virt machine GICv2 base addresses
#define GIC_DIST_BASE 0x08000000U
#define GIC_CPU_BASE 0x08010000U

// Register access macros
#define GIC_DIST_REG(offset) \
    (*(volatile uint32_t *)((uintptr_t)GIC_DIST_BASE + (offset)))

#define GIC_DIST_REG8(offset) \
    (*(volatile uint8_t *)((uintptr_t)GIC_DIST_BASE + (offset)))

#define GIC_CPU_REG(offset) \
    (*(volatile uint32_t *)((uintptr_t)GIC_CPU_BASE + (offset)))

// Distributor registers
#define GIC_DIST_CTRL GIC_DIST_REG(0x000)
#define GIC_DIST_IGROUPR0 GIC_DIST_REG(0x080)
#define GIC_DIST_ENABLE0 GIC_DIST_REG(0x100)
#define GIC_DIST_PRIORITY 0x400

// CPU Interface registers
#define GIC_CPU_CTRL GIC_CPU_REG(0x000)
#define GIC_CPU_PMR GIC_CPU_REG(0x004)
#define GIC_CPU_IAR GIC_CPU_REG(0x00C)
#define GIC_CPU_EOI GIC_CPU_REG(0x010)
#pragma endregion

#pragma region virtq

#define VIRTIO_MMIO_BASE 0x0A000000U
#define VIRTIO_REG(offset) (*(volatile uint32_t *)((uintptr_t)VIRTIO_MMIO_BASE + (offset)))

/* Identification */
#define VIRTIO_MAGIC_VALUE VIRTIO_REG(0x000)
#define VIRTIO_VERSION VIRTIO_REG(0x004)
#define VIRTIO_DEVICE_ID VIRTIO_REG(0x008)
#define VIRTIO_VENDOR_ID VIRTIO_REG(0x00C)

/* Features */
#define VIRTIO_HOST_FEATURES VIRTIO_REG(0x010)
#define VIRTIO_HOST_FEATURES_SEL VIRTIO_REG(0x014)
#define VIRTIO_GUEST_FEATURES VIRTIO_REG(0x020)
#define VIRTIO_GUEST_FEATURES_SEL VIRTIO_REG(0x024)

/* Legacy Queue Configuration */
#define VIRTIO_GUEST_PAGE_SIZE VIRTIO_REG(0x028)
#define VIRTIO_QUEUE_SEL VIRTIO_REG(0x030)
#define VIRTIO_QUEUE_NUM_MAX VIRTIO_REG(0x034)
#define VIRTIO_QUEUE_NUM VIRTIO_REG(0x038)
#define VIRTIO_QUEUE_ALIGN VIRTIO_REG(0x03C)
#define VIRTIO_QUEUE_PFN VIRTIO_REG(0x040)
#define VIRTIO_QUEUE_NOTIFY VIRTIO_REG(0x050)

/* Interrupt */
#define VIRTIO_INTERRUPT_STATUS VIRTIO_REG(0x060)
#define VIRTIO_INTERRUPT_ACK VIRTIO_REG(0x064)

/* Device Status */
#define VIRTIO_STATUS VIRTIO_REG(0x070)

/* Status Flags */
#define VIRTIO_STATUS_ACKNOWLEDGE 0x01U
#define VIRTIO_STATUS_DRIVER 0x02U
#define VIRTIO_STATUS_DRIVER_OK 0x04U
#define VIRTIO_STATUS_FEATURES_OK 0x08U

/* Virtqueue Descriptor Flags */
#define VIRTQ_DESC_F_NEXT 0x01U
#define VIRTQ_DESC_F_WRITE 0x02U

#pragma endregion

#endif
