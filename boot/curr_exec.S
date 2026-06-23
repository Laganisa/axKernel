.global curr_el_sp0_sync
.global curr_el_sp0_irq
.global curr_el_sp0_fiq
.global curr_el_sp0_serror
.global curr_el_spx_sync
.global curr_el_spx_irq
.global curr_el_spx_fiq
.global curr_el_spx_serror

.extern current_task_id

curr_el_sp0_sync:
    bl check_inf_loop
    b hang

curr_el_sp0_irq:
    bl check_inf_loop
    b hang

curr_el_sp0_fiq:
    bl check_inf_loop
    b hang

curr_el_sp0_serror:
    bl check_inf_loop
    b hang

curr_el_spx_sync:

    bl check_sync
    bl reg_elr_el1
    bl reg_esr_el1
    bl reg_far_el1

    // 스택 공간 확보 및 컨텍스트 저장
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    
    mrs x9, elr_el1
    mrs x10, spsr_el1
    stp x9, x10, [sp, #256]

    mrs x0, esr_el1
    lsr x1, x0, #26

    cmp x1, #0x15           // SVC 예외인지 확인
    b.ne .panic_handler

    mov x0, x8              
    bl handle_syscall

    str x0, [sp, #248]

    ldp x9, x10, [sp, #256]
    msr elr_el1, x9
    msr spsr_el1, x10
    
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]

    ldr x0, [sp, #248]
    add sp, sp, #272
    eret

curr_el_spx_irq:
    bl check_irq
    sub sp, sp, #272
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]

    mrs x0, elr_el1
    mrs x1, spsr_el1
    stp x0, x1, [sp, #256]

    bl get_current_proc_addr

    mov x20, x0
    mov x0, x20
    mov x1, sp

    bl irq_handler_main       

    mov sp, x0              

    ldp x0, x1, [sp, #256]   // ELR, SPSR 복원
    msr elr_el1, x0
    msr spsr_el1, x1

    ldr x30, [sp, #240]      // x30 복원
    ldp x28, x29, [sp, #224]
    ldp x26, x27, [sp, #208]
    ldp x24, x25, [sp, #192]
    ldp x22, x23, [sp, #176]
    ldp x20, x21, [sp, #160]
    ldp x18, x19, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8, x9, [sp, #64]
    ldp x6, x7, [sp, #48]
    ldp x4, x5, [sp, #32]
    ldp x2, x3, [sp, #16]
    ldp x0, x1, [sp, #0]

    // add sp, sp, #272
    add sp, sp, #272
    eret

curr_el_spx_fiq:
    bl check_inf_loop
    b hang

curr_el_spx_serror:
    bl check_inf_loop
    b hang

.panic_handler:
    b .panic_handler

hang:
    b hang