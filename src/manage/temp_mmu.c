#include "manage/temp_mm.h"
#include "global/_io.h"

#define TEMP_MMU_MAX_PROCESSES 8
#define TEMP_MMU_L1_ENTRIES 512
#define TEMP_MMU_L2_ENTRIES 512
#define TEMP_MMU_L3_ENTRIES 512
#define TEMP_MMU_BLOCK_SIZE (1ULL << 30)
#define TEMP_MMU_L2_SIZE (1ULL << 21)
#define TEMP_MMU_KERNEL_IDENTITY_ENTRIES 4

#define TEMP_MMU_ATTR_DEVICE (1ULL << 2)
#define TEMP_MMU_ATTR_NORMAL 0ULL
#define TEMP_MMU_AP_USER_RW (1ULL << 6)
#define TEMP_MMU_AP_USER_RO (3ULL << 6)
#define TEMP_MMU_SH_INNER (3ULL << 8)
#define TEMP_MMU_AF (1ULL << 10)
#define TEMP_MMU_PXN (1ULL << 53)
#define TEMP_MMU_UXN (1ULL << 54)
#define TEMP_MMU_TABLE_ADDRESS_MASK 0x0000FFFFFFFFF000ULL
#define TEMP_MMU_L1_ADDRESS_MASK 0x000000FFC0000000ULL
#define TEMP_MMU_PAGE_ADDRESS_MASK 0x0000FFFFFFFFF000ULL
#define TEMP_MMU_L1_BLOCK_DESCRIPTOR 0x1ULL
#define TEMP_MMU_L2_TABLE_DESCRIPTOR 0x3ULL
#define TEMP_MMU_PAGE_DESCRIPTOR 0x3ULL

#define TEMP_MMU_TCR_VALUE                                        \
    (25ULL | (1ULL << 8) | (1ULL << 10) | (3ULL << 12) |          \
     (16ULL << 16) | (1ULL << 23) | (1ULL << 24) | (1ULL << 26) | \
     (3ULL << 28) | (2ULL << 30) | (2ULL << 32))

static uint64_t temp_mmu_kernel_l1[TEMP_MMU_L1_ENTRIES]
    __attribute__((aligned(4096)));
static uint64_t temp_mmu_proc_l1[TEMP_MMU_MAX_PROCESSES][TEMP_MMU_L1_ENTRIES]
    __attribute__((aligned(4096)));
static uint64_t temp_mmu_proc_l2[TEMP_MMU_MAX_PROCESSES][TEMP_MMU_L2_ENTRIES]
    __attribute__((aligned(4096)));
static uint64_t temp_mmu_proc_l3[TEMP_MMU_MAX_PROCESSES][TEMP_MMU_L3_ENTRIES]
    __attribute__((aligned(4096)));
static uint8_t temp_mmu_proc_ready[TEMP_MMU_MAX_PROCESSES];
static uint8_t temp_mmu_id_to_slot[MAX_PCB_SIZE];
static uint8_t temp_mmu_ready;

static uint64_t temp_mmu_l1_block(uint64_t phys_addr, uint64_t attributes)
{
    return (phys_addr & TEMP_MMU_L1_ADDRESS_MASK) |
           TEMP_MMU_L1_BLOCK_DESCRIPTOR | attributes;
}

static void temp_mmu_write_ttbr0(uint64_t table_addr)
{
    asm volatile(
        "dsb ishst\n"
        "msr ttbr0_el1, %0\n"
        "isb\n"
        "tlbi vmalle1\n"
        "dsb ish\n"
        "isb\n"
        :
        : "r"(table_addr)
        : "memory");
}

void temp_mmu_init(void)
{
    if (temp_mmu_ready)
    {
        return;
    }

    for (uint16_t i = 0; i < TEMP_MMU_L1_ENTRIES; i++)
    {
        temp_mmu_kernel_l1[i] = 0;
    }
    for (uint16_t i = 0; i < MAX_PCB_SIZE; i++)
    {
        temp_mmu_id_to_slot[i] = 0xFF;
    }

    uint64_t device_attributes =
        TEMP_MMU_ATTR_DEVICE | TEMP_MMU_AF | TEMP_MMU_PXN | TEMP_MMU_UXN;
    uint64_t kernel_attributes =
        TEMP_MMU_ATTR_NORMAL | TEMP_MMU_SH_INNER | TEMP_MMU_AF | TEMP_MMU_UXN;

    temp_mmu_kernel_l1[0] = temp_mmu_l1_block(0, device_attributes);
    for (uint16_t i = 1; i < TEMP_MMU_KERNEL_IDENTITY_ENTRIES; i++)
    {
        temp_mmu_kernel_l1[i] =
            temp_mmu_l1_block((uint64_t)i * TEMP_MMU_BLOCK_SIZE, kernel_attributes);
    }

    uint64_t mair = 0x04FFULL;
    uint64_t tcr = TEMP_MMU_TCR_VALUE;
    uint64_t ttbr0 = (uint64_t)(uintptr_t)temp_mmu_kernel_l1;

    asm volatile("dsb sy" ::: "memory");
    asm volatile("msr mair_el1, %0" : : "r"(mair) : "memory");
    asm volatile("msr tcr_el1, %0" : : "r"(tcr) : "memory");
    asm volatile("msr ttbr0_el1, %0" : : "r"(ttbr0) : "memory");
    asm volatile(
        "isb\n"
        "tlbi vmalle1\n"
        "dsb sy\n"
        "isb\n"
        :
        :
        : "memory");

    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr = (sctlr | 1ULL) & ~(1ULL << 1);
    asm volatile("msr sctlr_el1, %0\nisb" : : "r"(sctlr) : "memory");

    temp_mmu_ready = TRUE;
}

uint8_t temp_mmu_prepare_user(
    pcb_t *proc,
    page_t *software_root,
    uint64_t phys_base,
    uint64_t user_base,
    uint64_t size,
    const uint8_t *page_flags)
{

    if (!temp_mmu_ready || proc == NULL || software_root == NULL ||
        page_flags == NULL || proc->id == 0 ||
        (phys_base & (TEMP_MM_PAGE_SIZE - 1)) != 0 ||
        (user_base & (TEMP_MM_PAGE_SIZE - 1)) != 0 ||
        size == 0 || (size & (TEMP_MM_PAGE_SIZE - 1)) != 0 ||
        phys_base >= (1ULL << 40) || size > (1ULL << 40) - phys_base ||
        user_base >= (1ULL << 39) || size > (1ULL << 39) - user_base ||
        (user_base & (TEMP_MMU_L2_SIZE - 1)) + size > TEMP_MMU_L2_SIZE)
    {

        return FALSE;
    }

    uint16_t id = proc->id;
    if (temp_mmu_id_to_slot[id] != 0xFF)
    {
        return FALSE;
    }

    uint16_t slot = TEMP_MMU_MAX_PROCESSES;
    for (uint16_t i = 0; i < TEMP_MMU_MAX_PROCESSES; i++)
    {
        if (!temp_mmu_proc_ready[i])
        {
            slot = i;
            break;
        }
    }
    if (slot == TEMP_MMU_MAX_PROCESSES)
    {
        return FALSE;
    }

    uint64_t *l1 = temp_mmu_proc_l1[slot];
    uint64_t *l2 = temp_mmu_proc_l2[slot];
    uint64_t *l3 = temp_mmu_proc_l3[slot];
    for (uint16_t i = 0; i < TEMP_MMU_L1_ENTRIES; i++)
    {
        l1[i] = temp_mmu_kernel_l1[i];
    }
    for (uint16_t i = 0; i < TEMP_MMU_L2_ENTRIES; i++)
    {
        l2[i] = 0;
        l3[i] = 0;
    }

    uint16_t l1_index = (uint16_t)((user_base >> 30) & 0x1FF);
    uint16_t l2_index = (uint16_t)((user_base >> 21) & 0x1FF);
    if (l1[l1_index] != 0)
    {
        return FALSE;
    }

    l1[l1_index] =
        ((uint64_t)(uintptr_t)l2 & TEMP_MMU_TABLE_ADDRESS_MASK) |
        TEMP_MMU_L2_TABLE_DESCRIPTOR;
    l2[l2_index] =
        ((uint64_t)(uintptr_t)l3 & TEMP_MMU_TABLE_ADDRESS_MASK) |
        TEMP_MMU_L2_TABLE_DESCRIPTOR;

    uint64_t page_count = size / TEMP_MM_PAGE_SIZE;
    uint64_t mapped_count = 0;
    for (; mapped_count < page_count; mapped_count++)
    {
        uint8_t flags = page_flags[mapped_count];
        if ((flags & (TEMP_MM_PAGE_WRITE | TEMP_MM_PAGE_EXEC)) ==
            (TEMP_MM_PAGE_WRITE | TEMP_MM_PAGE_EXEC))
        {
            break;
        }

        uint64_t vaddr = user_base + mapped_count * TEMP_MM_PAGE_SIZE;
        uint64_t paddr = phys_base + mapped_count * TEMP_MM_PAGE_SIZE;
        uint16_t l3_index = (uint16_t)((vaddr >> 12) & 0x1FF);
        uint64_t descriptor =
            (paddr & TEMP_MMU_PAGE_ADDRESS_MASK) |
            TEMP_MMU_PAGE_DESCRIPTOR | TEMP_MMU_AF | TEMP_MMU_PXN |
            TEMP_MMU_SH_INNER;

        if (flags & TEMP_MM_PAGE_WRITE)
        {
            descriptor |= TEMP_MMU_AP_USER_RW;
        }
        else
        {
            descriptor |= TEMP_MMU_AP_USER_RO;
        }
        if (!(flags & TEMP_MM_PAGE_EXEC))
        {
            descriptor |= TEMP_MMU_UXN;
        }

        if (!temp_mm_page_map(software_root, vaddr, paddr))
        {
            break;
        }
        l3[l3_index] = descriptor;
    }

    if (mapped_count != page_count)
    {
        for (uint64_t i = 0; i < mapped_count; i++)
        {
            uint16_t l3_index =
                (uint16_t)(((user_base + i * TEMP_MM_PAGE_SIZE) >> 12) & 0x1FF);
            l3[l3_index] = 0;
        }
        l1[l1_index] = 0;
        l2[l2_index] = 0;
        temp_mm_page_reset(software_root);
        return FALSE;
    }

    temp_mmu_proc_ready[slot] = TRUE;
    temp_mmu_id_to_slot[id] = (uint8_t)slot;
    asm volatile("dsb ishst\nisb" ::: "memory");
    return TRUE;
}

void temp_mmu_switch(pcb_t *proc)
{
    if (!temp_mmu_ready)
    {
        return;
    }

    uint64_t table_addr = (uint64_t)(uintptr_t)temp_mmu_kernel_l1;
    if (proc != NULL && proc->id != 0 &&
        temp_mmu_id_to_slot[proc->id] != 0xFF)
    {
        uint8_t slot = temp_mmu_id_to_slot[proc->id];
        if (temp_mmu_proc_ready[slot])
        {
            table_addr = (uint64_t)(uintptr_t)temp_mmu_proc_l1[slot];
        }
    }

    temp_mmu_write_ttbr0(table_addr);
}
