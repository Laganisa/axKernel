.global el1_spx_sync
.global el1_spx_irq
.global el1_spx_fiq
.global el1_spx_serror


.equ OFF_TEMP_X,        0
.equ OFF_TEMP_EXCE,     240     // 30 * 8
.equ OFF_BASE,          304     // 240 + 64


.extern current_task_id

// 여기도 바꾼 방식으로 수정하기
el1_spx_sync:

    bl check_el1_sync
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
    b.ne hang

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

// 나중에 고칠거
el1_spx_irq:
    ldr x9, =0x40024000
    
    stp x0, x1, [x9, #0]

    ldr x10, =current_proc
    ldr x10, [x10]

    stp x2,  x3,  [x10, #(2*8)]
    stp x4,  x5,  [x10, #(4*8)]
    stp x6,  x7,  [x10, #(6*8)]
    stp x8,  x9,  [x10, #(8*8)]   
    
    mov x11, x10
    stp x10, x12, [x11, #(10*8)]
    stp x13, x14, [x11, #(12*8)]
    stp x15, x16, [x11, #(14*8)]
    stp x17, x18, [x11, #(16*8)]
    stp x19, x20, [x11, #(18*8)]
    stp x21, x22, [x11, #(20*8)]
    stp x23, x24, [x11, #(22*8)]
    stp x25, x26, [x11, #(24*8)]
    stp x27, x28, [x11, #(26*8)]
    stp x29, x30, [x11, #(28*8)]

    mov x12, sp
    str x12, [x11, #(31*8)]

    mrs x12, spsr_el1
    str x12, [x11, #(32*8)]

    mrs x12, elr_el1
    str x12, [x11, #(33*8)]

    ldp x0, x1, [x9, #0]
    stp x0, x1, [x11, #(0*8)]

    mov x0, x11
    bl irq_handler_main

    b _proc

el1_spx_fiq:
    bl check_inf_loop
    b hang

el1_spx_serror:
    bl check_inf_loop
    b hang

hang:
    b hang