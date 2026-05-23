#include "../include/types.h"

#ifndef __SECT_H__
#define __SECT_H__

#pragma region QEMU_
// QEMU virt 머신의 PL011 UART 주소
#define UART0_BASE 0x09000000
#define UART0_DR ((volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_FR ((volatile uint32_t *)(UART0_BASE + 0x18))
#define UART0_CR ((volatile uint32_t *)(UART0_BASE + 0x30))

// 인터럽트 해석 QEMU virt machine GIC V2 주소
#define GIC_DIST_BASE 0x08000000 // Distributor
#define GIC_CPU_BASE 0x08010000  // CPU Interface

// 인터럽트 제어 레지스터
#define GICC_IAR ((volatile uint32_t *)(GIC_CPU_BASE + 0x0C))
#define GICC_EOI ((volatile uint32_t *)(GIC_CPU_BASE + 0x10))
#define GICC_CTLR ((volatile uint32_t *)(GIC_CPU_BASE + 0x00))
#define GICD_CTLR ((volatile uint32_t *)(GIC_DIST_BASE + 0x00))
#define GICD_ISENABLER ((volatile uint32_t *)(GIC_DIST_BASE + 0x100))

// 인터럽트 확인 레지스터
#define GIC_INTERFACE_IAR (*(volatile uint32_t *)(GIC_CPU_BASE + 0x0C))

// 인터럽트 종료 알림 레지스터
#define GIC_INTERFACE_EOI (*(volatile uint32_t *)(GIC_CPU_BASE + 0x10))

#pragma endregion

#define ALIGN_16(addr) (((addr) + 15) & ~15)

#pragma region KERNEL
// 커널의 메모리 정하기
#define KERNEL_TEXT_START 0x40000000
#define KERNEL_TEXT KERNEL_TEXT_START

#define KERNEL_TEXT_SIZE 0x19000 // 100KB정도 나중에 줄일 예정

#define KERNEL_TEXT_END (KERNEL_TEXT_START + KERNEL_TEXT_SIZE)

// KERNEL_DATA_START

#define MM_ADDR_SIZE 0x5000
#define PM_ADDR_SIZE 0x7000
#define FM_ADDR_SIZE 0x50000

#define MM_ADDR_START ALIGN_16(KERNEL_TEXT_END)
#define MM_ADDR_END ALIGN_16(MM_ADDR_START + MM_ADDR_SIZE)

#define PM_ADDR_START ALIGN_16(MM_ADDR_END)
#define PM_ADDR_END ALIGN_16(PM_ADDR_START + PM_ADDR_SIZE)

#define FM_ADDR_START ALIGN_16(PM_ADDR_END)
#define FM_ADDR_END ALIGN_16(FM_ADDR_START + FM_ADDR_SIZE)

// KERNEl_DATA_END

#define KERNEL_DATA_END FM_ADDR_END

#define KERNEL_BSS_START KERNEL_DATA_END
#define KERNEL_BSS_END KERNEL_BSS_START

#define KERNEL_STACK_START KERNEL_BSS_END
#define KERNEL_STACK_END KERNEL_STACK_START + 0x10000 // 64KB 스택

#pragma endregion

#pragma region USER

#define USER_PROC_START KERNEL_STACK_END
#define USER_PROC_END USER_PROC_START + 0x10000000 // 256MB 프로세스 공간

#define USER_FILE_START USER_PROC_END
#define USER_FILE_END USER_FILE_START + 0x88755000 // 2184MB 파일 공간

#pragma endregion

#endif
