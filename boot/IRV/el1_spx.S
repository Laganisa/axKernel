.global el1_spx_sync
.global el1_spx_irq
.global el1_spx_fiq
.global el1_spx_serror


.equ OFF_TEMP_X,        0
.equ OFF_TEMP_EXCE,     240     // 30 * 8
.equ OFF_BASE,          304     // 240 + 64


.extern current_task_id

// ! 여기도 바꾼 방식으로 수정하기
el1_spx_sync:
    ldr x9, =current_proc
    ldr x9, [x9]

    // 저장부
    stp x0,  x1,  [x9, #(0 * 8)]
    stp x2,  x3,  [x9, #(2 * 8)]
    stp x4,  x5,  [x9, #(4 * 8)]
    stp x6,  x7,  [x9, #(6 * 8)]
    stp x8,  x10, [x9, #(8 * 8)] 
    stp x11, x12, [x9, #(10 * 8)]
    stp x13, x14, [x9, #(12 * 8)]
    stp x15, x16, [x9, #(14 * 8)]
    stp x17, x18, [x9, #(16 * 8)]
    stp x19, x20, [x9, #(18 * 8)]
    stp x21, x22, [x9, #(20 * 8)]
    stp x23, x24, [x9, #(22 * 8)]
    stp x25, x26, [x9, #(24 * 8)]
    stp x27, x28, [x9, #(26 * 8)]
    str x29,      [x9, #(28 * 8)]
    str x30,      [x9, #(29 * 8)]
 
    mrs x10, spsr_el1
    str x10, [x9, #(32 * 8)] 
    mrs x10, elr_el1
    str x10, [x9, #(33 * 8)] 

    // 시스템 콜
    mov x0, x8
    ldr x1, [x9, #0]
    ldr x2, [x9, #8]
    ldr x3, [x9, #16]

    mrs x4, esr_el1
    lsr x4, x4, #26

    bl sync_handler_main
    
    ldr x9, =current_proc
    ldr x9, [x9]

    str x0, [x9, #0]

    ldp x0,  x1,  [x9, #(0 * 8)]
    ldp x2,  x3,  [x9, #(2 * 8)]
    ldp x4,  x5,  [x9, #(4 * 8)]
    ldp x6,  x7,  [x9, #(6 * 8)]
    ldp x8,  x10, [x9, #(8 * 8)]  
    ldp x11, x12, [x9, #(10 * 8)]
    ldp x13, x14, [x9, #(12 * 8)]
    ldp x15, x16, [x9, #(14 * 8)]
    ldp x17, x18, [x9, #(16 * 8)]
    ldp x19, x20, [x9, #(18 * 8)]
    ldp x21, x22, [x9, #(20 * 8)]
    ldp x23, x24, [x9, #(22 * 8)]
    ldp x25, x26, [x9, #(24 * 8)]
    ldp x27, x28, [x9, #(26 * 8)]
    ldr x29,      [x9, #(28 * 8)]
    ldr x30,      [x9, #(29 * 8)]
    
    ldr x10, [x9, #(32 * 8)] 
    msr spsr_el1, x10

    ldr x10, [x9, #(33 * 8)] 
    msr elr_el1, x10

    isb // 파이프라인 동기화

    eret


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