#ifndef __MACRO_H__
#define __MACRO_H__

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

#define VIRTQ_BASE 0x08000000

#pragma endregion

#endif
