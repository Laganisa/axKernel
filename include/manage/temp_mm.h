#ifndef __KERNEL_TEMP_MM_H__
#define __KERNEL_TEMP_MM_H__

#include "manage/_mm.h"
#include "manage/_pm.h"

#define TEMP_MM_PAGE_SIZE 4096ULL
#define TEMP_MM_USER_BASE 0x0000000100000000ULL

#define TEMP_MM_PAGE_READ 0x01
#define TEMP_MM_PAGE_WRITE 0x02
#define TEMP_MM_PAGE_EXEC 0x04

uint8_t temp_mm_page_map(page_t *root, uint64_t vaddr, uint64_t frame_addr);
uint8_t temp_mm_page_translate(
    page_t *root,
    uint64_t vaddr,
    uint64_t *paddr_out);
void temp_mm_page_reset(page_t *root);

void temp_mmu_init(void);
uint8_t temp_mmu_prepare_user(
    pcb_t *proc,
    page_t *software_root,
    uint64_t phys_base,
    uint64_t user_base,
    uint64_t size,
    const uint8_t *page_flags);
void temp_mmu_switch(pcb_t *proc);

#endif
