#include "../include/types.h"
#include "../include/io.h"
#include "../include/irq.h"
#include "../include/exce.h"
#include "../include/syscall.h"

// EL1, SP0 예외 정상일땐 작동 안함
void curr_el_sp0_sync()
{
    while (1)
        ;
}

void curr_el_sp0_irq()
{
    while (1)
        ;
}

void curr_el_sp0_fiq()
{
    while (1)
        ;
}

void curr_el_sp0_serror()
{
    while (1)
        ;
}

// EL1, SPx 예외 정상 작동중
void curr_el_spx_sync()
{
    puts("curr_el");
    uint64_t esr;
    uint64_t syscall_num;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    uint32_t ec = (esr >> 26) & 0x3F; // Exception Class

    if (ec == 0x15) // SVC instruction
    {
        asm volatile("mov %0, x8" : "=r"(syscall_num));
        asm volatile("mov %0, x0" : "=r"(arg1));
        asm volatile("mov %0, x1" : "=r"(arg2));
        asm volatile("mov %0, x2" : "=r"(arg3));
        handle_syscall(syscall_num, arg1, arg2, arg3);
    }
}

// extern pcb_t *current_proc;

void curr_el_spx_irq()
{

    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    uint32_t irq_nr = iar & 0x3FF;

    if (irq_nr == 30)
    {
        // 타이머만 재설정
        asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));
        // 부르더라도 '스케줄링' 로직이 없는 별도의 핸들러를 써야 함.
    }

    *(volatile uint32_t *)(GIC_CPU_BASE + 0x10) = iar;
    // 그대로 복귀 (iret/ret)
}

void curr_el_spx_fiq()
{
    while (1)
        ;
}

void curr_el_spx_serror()
{
    while (1)
        ;
}

// 하위 EL (AArch64)예외
void lower_el_aarch64_sync()
{
    while (1)
        ;
}

void lower_el_aarch64_fiq()
{
    while (1)
        ;
}

void lower_el_aarch64_serror()
{
    while (1)
        ;
}

// 하위 EL (AArch32) 예외
void lower_el_aarch32_sync()
{
    while (1)
        ;
}

void lower_el_aarch32_irq()
{
    while (1)
        ;
}

void lower_el_aarch32_fiq()
{
    while (1)
        ;
}

void lower_el_aarch32_serror()
{
    while (1)
        ;
}
