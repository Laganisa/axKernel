#include "../include/types.h"
#include "../include/io.h"
#include "../include/irq.h"
#include "../include/exce.h"
#include "../include/syscall.h"

#include "debug.h"

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
void curr_el_spx_sync(void) __attribute__((naked));
void curr_el_spx_sync(void)
{
    asm volatile(
        "sub sp, sp, #272\n"
        "stp x0, x1, [sp, #0]\n"
        "stp x2, x3, [sp, #16]\n"
        "stp x4, x5, [sp, #32]\n"
        "stp x6, x7, [sp, #48]\n"
        "stp x8, x9, [sp, #64]\n"
        "stp x10, x11, [sp, #80]\n"
        "stp x12, x13, [sp, #96]\n"
        "stp x14, x15, [sp, #112]\n"
        "stp x16, x17, [sp, #128]\n"
        "stp x18, x19, [sp, #144]\n"
        "stp x20, x21, [sp, #160]\n"
        "stp x22, x23, [sp, #176]\n"
        "stp x24, x25, [sp, #192]\n"
        "stp x26, x27, [sp, #208]\n"
        "stp x28, x29, [sp, #224]\n"
        "str x30, [sp, #240]\n"
        "mrs x9, elr_el1\n"
        "mrs x10, spsr_el1\n"
        "stp x9, x10, [sp, #256]\n"

        // ESR에서 Exception Class 검사 (SVC = 0x15)
        "mrs x11, esr_el1\n"
        "lsr x12, x11, #26\n"
        "and x12, x12, #0x3F\n"
        "mov x13, #0x15\n"
        "cmp x12, x13\n"
        "b.ne 1f\n"

        // syscall 인자: x8=syscall_num, x0..x2=args
        "mov x0, x8\n"
        "ldr x1, [sp, #0]\n"
        "ldr x2, [sp, #16]\n"
        "ldr x3, [sp, #32]\n"
        "bl handle_syscall\n"

        // 결과를 임시 저장
        "str x0, [sp, #248]\n"

        // 레지스터 복원
        "ldp x1, x2, [sp, #0]\n"
        "ldp x3, x4, [sp, #16]\n"
        "ldp x5, x6, [sp, #32]\n"
        "ldp x7, x8, [sp, #48]\n"
        "ldp x9, x10, [sp, #64]\n"
        "ldp x11, x12, [sp, #80]\n"
        "ldp x13, x14, [sp, #96]\n"
        "ldp x15, x16, [sp, #112]\n"
        "ldp x17, x18, [sp, #128]\n"
        "ldp x19, x20, [sp, #144]\n"
        "ldp x21, x22, [sp, #160]\n"
        "ldp x23, x24, [sp, #176]\n"
        "ldp x25, x26, [sp, #192]\n"
        "ldp x27, x28, [sp, #208]\n"
        "ldp x29, x30, [sp, #224]\n"

        "ldr x0, [sp, #248]\n"
        "ldp x1, x2, [sp, #256]\n"
        "msr elr_el1, x1\n"
        "msr spsr_el1, x2\n"
        "add sp, sp, #272\n"
        "eret\n"

        "1:\n"
        "// 예상치 못한 예외 클래스인 경우 무한 루프 (디버그용)\n"
        "b 1b\n"
        :
        :
        : "memory");
}

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
void lower_el_aarch64_sync(void) __attribute__((naked));
void lower_el_aarch64_sync(void)
{
    asm volatile(
        "sub sp, sp, #272\n"
        "stp x0, x1, [sp, #0]\n"
        "stp x2, x3, [sp, #16]\n"
        "stp x4, x5, [sp, #32]\n"
        "stp x6, x7, [sp, #48]\n"
        "stp x8, x9, [sp, #64]\n"
        "stp x10, x11, [sp, #80]\n"
        "stp x12, x13, [sp, #96]\n"
        "stp x14, x15, [sp, #112]\n"
        "stp x16, x17, [sp, #128]\n"
        "stp x18, x19, [sp, #144]\n"
        "stp x20, x21, [sp, #160]\n"
        "stp x22, x23, [sp, #176]\n"
        "stp x24, x25, [sp, #192]\n"
        "stp x26, x27, [sp, #208]\n"
        "stp x28, x29, [sp, #224]\n"
        "str x30, [sp, #240]\n"
        "mrs x9, elr_el1\n"
        "mrs x10, spsr_el1\n"
        "stp x9, x10, [sp, #256]\n"

        "mov x0, x8\n"
        "ldr x1, [sp, #0]\n"
        "ldr x2, [sp, #16]\n"
        "ldr x3, [sp, #32]\n"
        "bl handle_syscall\n"
        "str x0, [sp, #248]\n" // handle_syscall의 반환값 저장

        // --- 여기서부터 정확한 복구 루틴 시작 ---
        "ldp x0, x1, [sp, #0]\n"
        "ldp x2, x3, [sp, #16]\n"
        "ldp x4, x5, [sp, #32]\n"
        "ldp x6, x7, [sp, #48]\n"
        "ldp x8, x9, [sp, #64]\n"
        "ldp x10, x11, [sp, #80]\n"
        "ldp x12, x13, [sp, #96]\n"
        "ldp x14, x15, [sp, #112]\n"
        "ldp x16, x17, [sp, #128]\n"
        "ldp x18, x19, [sp, #144]\n"
        "ldp x20, x21, [sp, #160]\n"
        "ldp x22, x23, [sp, #176]\n"
        "ldp x24, x25, [sp, #192]\n"
        "ldp x26, x27, [sp, #208]\n"
        "ldp x28, x29, [sp, #224]\n"
        "ldr x30, [sp, #240]\n"

        "ldr x0, [sp, #248]\n"     // 반환값 다시 x0에 로드
        "ldp x1, x2, [sp, #256]\n" // elr, spsr 복구
        "msr elr_el1, x1\n"
        "msr spsr_el1, x2\n"
        "add sp, sp, #272\n"
        "eret\n"
        :
        :
        : "memory");
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

// Called from exception path when a data abort from lower EL occurs.
// Logs FAR/ELR/ESR and terminates the current process, scheduling next.
void handle_data_abort(uint64_t far, uint64_t elr, uint64_t esr)
{
    /*
    reg_far_el1();
    reg_esr_el1();
    reg_elr_el1();
    */

    pcb_t **current_proc_ptr = (pcb_t **)get_current_proc_addr();
    pcb_t *current = *current_proc_ptr;
    pcb_t *next;

    pm_awake(&pm_object, 1, current);
    next = pm_run(&pm_object);
    current_proc = next;

    if (next != 0)
    {
        new_context(next->sp);
    }

    while (1)
        ;
}
