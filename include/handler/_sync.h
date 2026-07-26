#ifndef __KERNEL_EXCEPTION_H__
#define __KERNEL_EXCEPTION_H__

#include "manage/_pm.h"

extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);

uint64_t sync_handler_main(
    uint64_t sys_call,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t ec);

// 예외 핸들러 선언
uint64_t handle_unknown(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);

uint64_t handle_trap_wfi_wfe(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_mcr_mrc(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_mcrr_mrrc(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_ldc_stc(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_fp_simd(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_vmrs(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_ptr_auth(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_ill_exec_state(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_svc_a32(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_hvc_a32(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_smc_a32(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_svc_a64(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_hvc_a64(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_smc_a64(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_msr_mrs_sys_inst(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_sve_exce(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_trap_eret(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t handle_ptr_auth_fail(uint64_t arg8, uint64_t arg1, uint64_t arg2, uint64_t arg3);

uint64_t handle_data_abort(uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t handle_inst_abort(uint64_t, uint64_t, uint64_t, uint64_t);

#endif
