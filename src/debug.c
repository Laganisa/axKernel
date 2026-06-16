#include "types.h"
#include "debug.h"
#include "io.h"

void dump(const char *name, uint64_t val)
{
    puts("[Debug] ");
    puts(name);
    puts(" : ");
    put_hex(val);
    puts("\n");
}

void full_stop(void)
{
    while (1)
    {
        ;
    }
}

void enter(const char *name)
{
    puts("\n");
    puts("[Enter] ");
    puts(name);
    puts("\n");
}

void log(const char *name)
{
    puts("[Debug] ");
    puts(name);
    puts("\n");
}

void reg_x8(void)
{
    uint64_t val;

    asm volatile(
        "mov %0, x8"
        : "=r"(val));

    dump("x8", val);
}

void reg_elr_el1(void)
{
    uint64_t val;

    asm volatile(
        "mrs %0, elr_el1"
        : "=r"(val));

    dump("elr_el1", val);
}

void reg_esr_el1(void)
{
    uint64_t val;

    asm volatile(
        "mrs %0, esr_el1"
        : "=r"(val));

    dump("esr_el1", val);
}

void reg_far_el1(void)
{
    uint64_t val;

    asm volatile(
        "mrs %0, far_el1"
        : "=r"(val));

    dump("far_el1", val);
}

void reg_vbar(void)
{
    uint64_t vbar;
    asm volatile(
        "mrs %0, vbar_el1"
        : "=r"(vbar));
    dump("vbar_el1", vbar);
}