.global lower_el_aarch64_sync
.global lower_el_aarch64_irq
.global lower_el_aarch64_fiq
.global lower_el_aarch64_serror
.global lower_el_aarch32_sync
.global lower_el_aarch32_irq
.global lower_el_aarch32_fiq
.global lower_el_aarch32_serror

.extern current_task_id

lower_el_aarch64_sync:
    // bl check_sync
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

    mov x0, x8
    ldr x1, [sp, #0]
    ldr x2, [sp, #8]
    ldr x3, [sp, #16]

    mrs x11, esr_el1
    lsr x11, x11, #26

    cmp x11, #0x20
    b.eq is_inst_abort

    cmp x11, #0x15
    b.eq is_syscall

    cmp x11, #0x24
    b.eq is_data_abort
    
    b not_syscall

lower_el_aarch64_sync_p2:
    str x0, [sp, #248]

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
    ldp x1, x2, [sp, #256]

    msr elr_el1, x1
    msr spsr_el1, x2

    add sp, sp, #272
    eret
    
lower_el_aarch64_irq:

    sub sp, sp, #272

    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
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
    str x30,      [sp, #240]

    mrs x0, elr_el1
    mrs x1, spsr_el1
    stp x0, x1, [sp, #256]

    mov x22, sp                  
    bl get_current_proc_addr
    mov x20, x0                   
    cbz x20, no_current_proc      

    ldr x23, [x20, #80]         
    

    add x0, x22, #256            

loop_copy:
    ldp x1, x2, [x22], #16
    stp x1, x2, [x23], #16
    cmp x22, x0
    b.lt loop_copy

    
    mov x0, x20
    mov x1, x22                   // sp 전달
    bl irq_handler_main

    mov sp, x0

    // bl flow_asm
    
    ldp x0, x1, [sp, #256]
    msr elr_el1, x0
    msr spsr_el1, x1

    ldp x0,  x1,  [sp, #0]
    ldp x2,  x3,  [sp, #16]
    ldp x4,  x5,  [sp, #32]
    ldp x6,  x7,  [sp, #48]
    ldp x8,  x9,  [sp, #64]
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
    ldr x30,      [sp, #240]

    add sp, sp, #272
    eret

no_current_proc:
    mov sp, x19
    ldp x0, x1, [sp, #256]
    msr elr_el1, x0
    msr spsr_el1, x1
    ldr x30, [sp, #240]
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
    add sp, sp, #272
    eret

is_syscall:
    bl handle_syscall
    b lower_el_aarch64_sync_p2

not_syscall:
    // 예외 디버깅 일단 무한 루프 돌려두기
    bl check_irq
    b hang

is_data_abort:
    bl handle_data_abort
    bl check_inf_loop
    b hang

is_inst_abort:
    // bl check_inf_loop
    b hang

lower_el_aarch64_fiq:
    bl check_inf_loop
    b hang

lower_el_aarch64_serror:
    bl check_inf_loop
    b hang

lower_el_aarch32_sync:
    bl check_inf_loop
    b hang

lower_el_aarch32_irq:
    bl check_inf_loop
    b hang

lower_el_aarch32_fiq:
    bl check_inf_loop
    b hang

lower_el_aarch32_serror:
    bl check_inf_loop
    b hang

hang:
    b hang