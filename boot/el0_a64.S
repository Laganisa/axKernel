.global el0_a64_sync
.global el0_a64_irq
.global el0_a64_fiq
.global el0_a64_serror

.extern current_task_id

// 바꾼 방식으로 수정하기
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

    // 1. PCB 주소를 다시 가져옴 (x9이 베이스 주소)
    ldr x9, =current_proc
    ldr x9, [x9]

    // 2. 시스템 콜 리턴값(x0)을 PCB의 reg_x[0] 위치에 저장
    str x0, [x9, #0]

    // 3. 모든 범용 레지스터 복구 (PCB에서)
    ldp x0,  x1,  [x9, #(0 * 8)]
    ldp x2,  x3,  [x9, #(2 * 8)]
    ldp x4,  x5,  [x9, #(4 * 8)]
    ldp x6,  x7,  [x9, #(6 * 8)]
    ldp x8,  x10, [x9, #(8 * 8)]  // x9은 베이스주소이므로 x9레지스터 자체는 복구 안함
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

    // 4. PC와 SPSR 복원 (PCB에서)
    ldr x1, [x9, #(33 * 8)] // elr_el1
    ldr x2, [x9, #(32 * 8)] // spsr
    msr elr_el1, x1
    msr spsr_el1, x2
    
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
    
loop_copy:
    ldp x1, x2, [x22], #16
    stp x1, x2, [x23], #16
    cmp x22, x0
    b.lt loop_copy

    
    mov x0, x20
    mov x1, x22                   // sp 전달
    bl irq_handler_main

    mov sp, x0

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

not_syscall:
    // 예외 디버깅 일단 무한 루프 돌려두기
    bl check_el0_irq
    b hang

is_data_abort:
    bl handle_data_abort
    bl check_inf_loop
    b hang

is_inst_abort:
    bl check_inf_loop
    b hang

el0_a64_fiq:
    bl check_inf_loop
    b hang

el0_a64_serror:
    bl check_inf_loop
    b hang

hang:
    b hang