.global el0_a64_sync
.global el0_a64_irq
.global el0_a64_fiq
.global el0_a64_serror

.extern current_task_id

el0_a64_sync:
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

    mrs x10, sp_el0
    str x10, [x9, #(31 * 8)] 
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

    ldr x10, [x9, #(31 * 8)] 
    msr sp_el0, x10
    
    ldr x10, [x9, #(32 * 8)] 
    msr spsr_el1, x10

    ldr x10, [x9, #(33 * 8)] 
    msr elr_el1, x10

    isb // 파이프라인 동기화

    eret

el0_a64_irq:

    // 현재 프로세스의 포인터 주소를 받아옴 x9에
    ldr x9, =current_proc
    ldr x9, [x9]         

    // 저장부
    stp x0,  x1,  [x9, #(0 * 8)]
    stp x2,  x3,  [x9, #(2 * 8)]
    stp x4,  x5,  [x9, #(4 * 8)]
    stp x6,  x7,  [x9, #(6 * 8)]
    stp x8,  x10, [x9, #(8 * 8)] 
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

    // pcb 구조체에 저장하기
    mrs x10, sp_el0
    str x10, [x9, #(31 * 8)] 
    mrs x10, spsr_el1
    str x10, [x9, #(32 * 8)] 
    mrs x10, elr_el1
    str x10, [x9, #(33 * 8)] 

    // 핸들러 호출하기
    mov x0, x9           
    bl irq_handler_main

    // 복원부 호출
    b _proc

el0_a64_fiq:
    bl check_inf_loop
    b hang

el0_a64_serror:
    bl check_inf_loop
    b hang

hang:
    b hang