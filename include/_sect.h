#ifndef __KERNEL_SECT_H__
#define __KERNEL_SECT_H__

#include "_types.h"

#define ALIGN_16(addr) (((addr) + 15) & ~15ULL)

extern unsigned char __kernel_start[];
extern unsigned char __mm_addr_start[];
extern unsigned char __mm_addr_end[];
extern unsigned char __pm_addr_start[];
extern unsigned char __pm_addr_end[];
extern unsigned char __fm_addr_start[];
extern unsigned char __fm_addr_end[];
extern unsigned char __dm_addr_start[];
extern unsigned char __dm_addr_end[];
extern unsigned char __nm_addr_start[];
extern unsigned char __nm_addr_end[];
extern unsigned char __gm_addr_start[];
extern unsigned char __gm_addr_end[];
extern unsigned char __kernel_data_end[];
extern unsigned char __kernel_bss_start[];
extern unsigned char __kernel_bss_end[];
extern unsigned char __kernel_stack_start[];
extern unsigned char __kernel_stack_end[];
extern unsigned char __kernel_heap_start[];
extern unsigned char __kernel_heap_end[];
extern unsigned char __user_proc_start[];
extern unsigned char __user_proc_end[];
extern unsigned char __user_file_start[];
extern unsigned char __user_file_end[];

#define KERNEL_TEXT_START ((uint64_t)__kernel_start)
#define KERNEL_TEXT KERNEL_TEXT_START

#define MM_ADDR_SIZE 0x6000ULL
#define PM_ADDR_SIZE 0x1F000ULL
#define FM_ADDR_SIZE 0x50000ULL
#define DM_ADDR_SIZE 0x5000ULL
#define NM_ADDR_SIZE 0x5000ULL
#define GM_ADDR_SIZE 0x5000ULL

#define MM_ADDR_START ((uint64_t)__mm_addr_start)
#define MM_ADDR_END ((uint64_t)__mm_addr_end)
#define PM_ADDR_START ((uint64_t)__pm_addr_start)
#define PM_ADDR_END ((uint64_t)__pm_addr_end)
#define FM_ADDR_START ((uint64_t)__fm_addr_start)
#define FM_ADDR_END ((uint64_t)__fm_addr_end)
#define DM_ADDR_START ((uint64_t)__dm_addr_start)
#define DM_ADDR_END ((uint64_t)__dm_addr_end)
#define NM_ADDR_START ((uint64_t)__nm_addr_start)
#define NM_ADDR_END ((uint64_t)__nm_addr_end)
#define GM_ADDR_START ((uint64_t)__gm_addr_start)
#define GM_ADDR_END ((uint64_t)__gm_addr_end)

#define KERNEL_DATA_END ((uint64_t)__kernel_data_end)
#define KERNEL_BSS_SIZE 0x10000ULL
#define KERNEL_STACK_SIZE 0x10000ULL
#define KERNEL_HEAP_SIZE 0x80000ULL
#define KERNEL_BSS_START ((uint64_t)__kernel_bss_start)
#define KERNEL_BSS_END ((uint64_t)__kernel_bss_end)
#define KERNEL_STACK_START ((uint64_t)__kernel_stack_start)
#define KERNEL_STACK_END ((uint64_t)__kernel_stack_end)
#define KERNEL_HEAP_START ((uint64_t)__kernel_heap_start)
#define KERNEL_HEAP_END ((uint64_t)__kernel_heap_end)

#define USER_PROC_START ((uint64_t)__user_proc_start)
#define USER_PROC_SIZE 0x10000000ULL
#define USER_PROC_END ((uint64_t)__user_proc_end)
#define USER_FILE_START ((uint64_t)__user_file_start)
#define USER_FILE_SIZE 0x88755000ULL
#define USER_FILE_END ((uint64_t)__user_file_end)

#endif
