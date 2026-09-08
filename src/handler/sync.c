#include "global/_io.h"
#include "handler/_irq.h"
#include "handler/_sync.h"
#include "handler/_syscall.h"
#include "global/_debug.h"

/*
    헨들러 예외 처리를 C로 처리하는 파일
*/

// 알 수 없는 ec 명령
static uint64_t unknown_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("unknown_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t trap_wfi_wfe_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_wfi_wfe_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t trap_mcr_mrc_a32_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_mcr_mrc_a32_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t trap_mcrr_mrrc_a32_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_mcrr_mrrc_a32_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t trap_mrc_mcr_a32_coproc_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_mrc_mcr_a32_coproc_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t trap_ldc_stc_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_ldc_stc_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t trap_fp_simd_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_fp_simd_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t trap_vmrs_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_vmrs_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t trap_ptr_auth_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_ptr_auth_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t illegal_excu_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_ptr_auth_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t svc_a32_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("svc_a32_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t hvc_a32_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("hvc_a32_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t smc_a32_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("smc_a32_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t hvc_a64_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("hvc_a64_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t smc_a64_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("smc_a64_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t trap_msr_mrs_sysinst_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("trap_msr_mrs_sysinst_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t sve_exce_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("sve_exce_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t eret_trap_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("eret_trap_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t ptr_auth_fail_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("ptr_auth_fail_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t inst_abort_el0_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("inst_abort_el0_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t inst_abort_el1_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("inst_abort_el1_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t pc_align_fault_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("pc_align_fault_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t data_abort_el0_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("data_abort_el0_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t data_abort_el1_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("data_abort_el1_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t sp_align_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("sp_align_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t floatpoint_exce_a32_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("floatpoint_exce_a32_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t floatpoint_exce_a64_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("floatpoint_exce_a64_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t serror_irq_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("serror_irq_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t breakpoint_el0_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("breakpoint_el0_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t breakpoint_el0_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("breakpoint_el0_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t breakpoint_el1_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("breakpoint_el1_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t software_step_el0_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("software_step_el0_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t software_step_el1_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("software_step_el1_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t watchpoint_el0_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("watchpoint_el0_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}
static uint64_t watchpoint_el1_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("watchpoint_el1_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

static uint64_t brk_inst_a64_handle(
    uint64_t arg8,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    enter("brk_inst_a64_handle");

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    log("Kernel Panic!");

    full_stop();

    return 0;
}

uint64_t (*ec_table[64])(
    uint64_t,
    uint64_t,
    uint64_t,
    uint64_t,
    uint64_t,
    uint64_t) = {
    [0x00] = &unknown_handle,
    [0x01] = &trap_wfi_wfe_handle,
    [0x03] = &trap_mcr_mrc_a32_handle,
    [0x04] = &trap_mcrr_mrrc_a32_handle,
    [0x05] = &trap_mrc_mcr_a32_coproc_handle,
    [0x06] = &trap_ldc_stc_handle,
    [0x07] = &trap_fp_simd_handle,
    [0x08] = &trap_vmrs_handle,
    [0x09] = &trap_ptr_auth_handle,
    [0x0E] = &illegal_excu_handle,
    [0x11] = &svc_a32_handle,
    [0x12] = &hvc_a32_handle,
    [0x13] = &smc_a32_handle,
    [0x15] = &svc_a64_handle,
    [0x16] = &hvc_a64_handle,
    [0x17] = &smc_a64_handle,
    [0x18] = &trap_msr_mrs_sysinst_handle,
    [0x19] = &sve_exce_handle,
    [0x1A] = &eret_trap_handle,
    [0x1C] = &ptr_auth_fail_handle,
    [0x20] = &inst_abort_el0_handle,
    [0x21] = &inst_abort_el1_handle,
    [0x22] = &pc_align_fault_handle,
    [0x24] = &data_abort_el0_handle,
    [0x25] = &data_abort_el1_handle,
    [0x26] = &sp_align_handle,
    [0x28] = &floatpoint_exce_a32_handle,
    [0x2C] = &floatpoint_exce_a64_handle,
    [0x2F] = &serror_irq_handle,
    [0x30] = &breakpoint_el0_handle,
    [0x31] = &breakpoint_el1_handle,
    [0x32] = &software_step_el0_handle,
    [0x33] = &software_step_el1_handle,
    [0x34] = &watchpoint_el0_handle,
    [0x35] = &watchpoint_el1_handle,
    [0x3C] = &brk_inst_a64_handle};

uint64_t sync_handler_main(
    uint64_t sys_call,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t ec)
{
    // sync 리턴값을 받을 변수
    // 시스템 콜 리턴값을 받을 변수
    uint64_t val;

    // ec 값에 맞는 함수 실행
    val = ec_table[ec](
        sys_call,
        arg1,
        arg2,
        arg3,
        arg4,
        arg5);

    return val;
}