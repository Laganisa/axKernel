#include "global/_debug.h"
#include "global/_io.h"
#include "manage/_mm.h"

#define toggle TRUE

void dump(const char *name, uint64_t val)
{
    if (toggle)
    {
        puts("[Debug] ");
        puts(name);
        puts(" : ");
        put_hex(val);
        puts("\n");
    }
}

void dump_(const char *name, uint64_t val)
{
    if (toggle)
    {
        puts("\033[34m[Debug] ");
        puts(name);
        puts(" : ");
        put_hex(val);
        puts("\033[0m\n");
    }
}

void _dump(const char *name, uint64_t val)
{
    if (toggle)
    {
        puts("\033[36m[Debug] ");
        puts(name);
        puts(" : ");
        put_hex(val);
        puts("\033[0m\n");
    }
}

void for_dump(const char *name, uint64_t *val, uint8_t num)
{
    if (toggle)
    {
        for (int i = 0; i < num; i++)
        {
            puts("[Debug] ");
            puts(name);
            puts(" : ");
            put_hex(val[i]);
            puts("\n");
        }
    }
}

void for2_dump(const char *name, uint64_t *val, uint8_t num)
{
    if (!toggle || !val)
    {
        return;
    }

    puts("[Debug] ");
    puts(name ? name : "(null)");
    puts(" :\n");

    for (uint8_t i = 0; i < num; i++)
    {
        put_hex2(val[i]);
        puts(" ");
        if ((i + 1) % 5 == 0)
        {
            puts("\n");
        }
    }

    if (num % 5 != 0)
    {
        puts("\n");
    }
}

// 함수 내부에서 함수 호출 인자를 보는 함수
void xxx_dump() {}

void full_stop(void)
{
    disable_irq();

    puts("\ninf loop\n");

    while (1)
    {
        asm volatile("wfe");
    }
}

void enter(const char *name)
{
    if (toggle)
    {
        puts("\n");
        puts("\033[31m[Enter] ");
        puts(name);
        puts("\033[0m\n");
    }
}

void exit(const char *name)
{
    if (toggle)
    {
        puts("\n");
        puts("[Exit] ");
        puts(name);
        puts("\n");
    }
}

void log(const char *name)
{
    if (toggle)
    {
        puts("[Debug] ");
        puts(name);
        puts("\n");
    }
}

void flow(const uint8_t type)
{
    if (type == 0)
    {
        log("flow pass");
    }
    else
    {
        puts("[Debug] ");
        puts("flow test #");
        put_hex2(type);
        puts("\n");
    }
}

#pragma region regs

void reg_x8(void)
{
    if (toggle)
    {
        uint64_t val;

        asm volatile(
            "mov %0, x8"
            : "=r"(val));

        dump("x8", val);
    }
}

void reg_elr_el1(void)
{
    if (toggle)
    {
        uint64_t val;

        asm volatile(
            "mrs %0, elr_el1"
            : "=r"(val));

        dump("elr_el1", val);
    }
}

void reg_esr_el1(void)
{
    if (toggle)
    {
        uint64_t val;

        asm volatile(
            "mrs %0, esr_el1"
            : "=r"(val));

        dump("esr_el1", val);
    }
}

void reg_far_el1(void)
{
    if (toggle)
    {
        uint64_t val;

        asm volatile(
            "mrs %0, far_el1"
            : "=r"(val));

        dump("far_el1", val);
    }
}

void reg_vbar(void)
{
    if (toggle)
    {
        uint64_t vbar;
        asm volatile(
            "mrs %0, vbar_el1"
            : "=r"(vbar));
        dump("vbar_el1", vbar);
    }
}

void reg_cntfrq_el0(void)
{
    uint64_t freq;
    asm volatile(
        "mrs %0, cntfrq_el0"
        : "=r"(freq));
    dump("cntp_frq_el0", freq);
}

#pragma endregion

#pragma region check_used_asm

void check_el0_sync(void)
{
    log("el0_SYNC");
}

void check_el1_sync(void)
{
    log("el1_SYNC");
}

void check_el1_irq(void)
{
    log("el1_IRQ");
}

void check_el0_irq(void)
{
    log("el0_IRQ");
}

void check_loop(void)
{
    log("A loop");
}

void check_inf_loop(void)
{
    log("inf loop");
}

void flow_asm(void)
{
    static int cnt = 0;

    cnt++;

    puts("[Debug] ");
    puts("flow pass in asm ");
    put_hex(cnt);
    puts("\n");
}

#pragma endregion

void proc_dump(const char *name, pcb_t *proc)
{
    if (toggle)
    {
        puts("[Debug] ");
        puts(name);
        puts(" : ");
        put_hex((uint64_t)proc);
        puts("\n");

        puts("[Debug] ");
        puts(name);
        puts(" id : ");
        put_hex(proc->id);
        puts("\n");

        puts("[Debug] ");
        puts(name);
        puts(" sp : ");
        put_hex(proc->regs.sp);
        puts("\n");

        puts("[Debug] ");
        puts(name);
        puts(" elr_el1 : ");
        put_hex(proc->regs.elr_el1);
        puts("\n");

        puts("[Debug] ");
        puts(name);
        puts(" spsr: ");
        put_hex(proc->regs.spsr);
        puts("\n");
    }
}

static void page_dump_hex(uint64_t value)
{
    const char *hex = "0123456789ABCDEF";

    puts("0x");
    for (int shift = 60; shift >= 0; shift -= 4)
    {
        putchar(hex[(value >> shift) & 0xF]);
    }
}

static void page_dump_entries(page_t *page, uint64_t virtual_address, uint8_t level)
{
    if (page == NULL || level > 2)
    {
        return;
    }

    uint8_t shift = 30 - (level * 9);
    for (uint16_t i = 0; i < 512; i++)
    {
        uint64_t entry_virtual_address =
            virtual_address | ((uint64_t)i << shift);
        if (page->is_leaf)
        {
            if (page->frame[i] != 0)
            {
                page_dump_hex(page->frame[i]);
                puts(" : ");
                page_dump_hex(entry_virtual_address);
                puts("\n");
            }
        }
        else if (page->pages[i] != NULL)
        {
            page_dump_entries(page->pages[i], entry_virtual_address, level + 1);
        }
    }
}

/*
    프로세스의 물리 주소와 가상 주소 매핑을 출력
*/
void page_dump(pcb_t *proc)
{
    if (!toggle || proc == NULL || proc->page_i == NULL)
    {
        return;
    }

    page_dump_entries(proc->page_i, 0, 0);
}

void file_dump(const char *name, fcb_t *file)
{
    if (toggle)
    {
        puts("[Debug] ");
        puts(name);
        puts(" : ");
        put_hex((uint64_t)file);
        puts("\n");
        puts("[Debug] ");
        puts(name);
        puts(" id : ");
        put_hex(file->fid);
        puts("\n");
        puts("[Debug] ");
        puts(name);
        puts(" size : ");
        put_hex(file->lens);
        puts("\n");
    }
}