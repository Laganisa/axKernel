#include "debug.h"
#include "io.h"
#include "pm.h"

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

void full_stop(void)
{
    puts("\ninf loop\n");
    while (1)
    {
        ;
    }
}

void enter(const char *name)
{
    if (toggle)
    {
        puts("\n");
        puts("[Enter] ");
        puts(name);
        puts("\n");
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

void flow(const char type)
{
    if (type == 0)
    {
        log("flow pass");
    }
    else if (type == 1)
    {
        puts("[Debug] ");
        puts("flow test #1");
        puts("\n");
    }
    else
    {
        puts("[Debug] ");
        puts("flow test #2");
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

void check_sync(void)
{
    log("SYNC");
}

void check_irq(void)
{
    log("IRQ");
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
        put_hex(proc);
        puts("\n");
        puts("[Debug] ");
        puts(name);
        puts(" id : ");
        put_hex(proc->id);
        puts("\n");
        puts("[Debug] ");
        puts(name);
        puts(" sp : ");
        put_hex(proc->sp);
        puts("\n");
        puts("[Debug] ");
        puts(name);
        puts(" elr_el1 : ");
        put_hex(proc->elr_el1);
        puts("\n");
    }
}