#include "global/_io.h"
#include "handler/_sync.h"
#include "global/_meta.h"
#include "_macro.h"

void gic_enable_irq(uint32_t irq)
{
    volatile uint32_t *reg =
        (volatile uint32_t *)(GIC_DIST_BASE + 0x100 + (irq / 32) * 4);

    *reg = (1U << (irq % 32));
}
#define GIC_DIST_DISABLE0 (*(volatile uint32_t *)(GIC_DIST_BASE + 0x180))

void gic_disable_irq(uint32_t irq)
{
    volatile uint32_t *reg =
        (volatile uint32_t *)(GIC_DIST_BASE + 0x180 + (irq / 32) * 4);

    *reg = (1U << (irq % 32));
}

void gic_set_priority(uint32_t irq, uint8_t priority)
{
    GIC_DIST_REG8(GIC_DIST_PRIORITY + irq) = priority;
}

void gic_init(void)
{
    GIC_DIST_CTRL = 1;

    // IRQ 79 → CPU 0
    GIC_DIST_REG8(0x84F) = 0x01;

    // IRQ 79 enable
    GIC_DIST_REG(0x108) |= (1U << 15);

    /* Distributor Disable */
    // GIC_DIST_CTRL = 0;

    /* 모든 PPI를 Group0 */
    GIC_DIST_IGROUPR0 = 0x00000000;

    /* Priority */
    for (uint32_t i = 16; i < 32; i++)
    {
        gic_set_priority(i, 0x80);
    }

    /* Enable PPI */
    GIC_DIST_ENABLE0 = 0xFFFF0000;

    /* Timer IRQ */
    gic_set_priority(NSPTI, 0xA0);
    gic_enable_irq(NSPTI);

    /* Distributor Enable */
    GIC_DIST_CTRL = 1;

    /* CPU Interface */
    GIC_CPU_CTRL = 1;
    GIC_CPU_PMR = 0xFF;

    asm volatile("dsb sy");
    asm volatile("isb");

    dump("GIC DIST CTRL",
         GIC_DIST_CTRL);

    dump("GIC IRQ79 ENABLE",
         GIC_DIST_REG(0x108) & (1U << 15));

    dump("GIC IRQ79 TARGET",
         GIC_DIST_REG8(0x84F));

    enable_irq();
}