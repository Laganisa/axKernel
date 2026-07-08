.align 11 // 테이블 시작 주소 2048바이트 정렬
.global vector_table
.global new_context

.extern current_task_id // 나중에 주석 달아서 없에기

vector_table:
    .align 7
    b el1_sp0_sync
    .align 7
    b el1_sp0_irq
    .align 7
    b el1_sp0_fiq
    .align 7
    b el1_sp0_serror

    .align 7
    b el1_spx_sync
    .align 7
    b el1_spx_irq  
    .align 7
    b el1_spx_fiq
    .align 7
    b el1_spx_serror

    .align 7
    b el0_a64_sync 
    .align 7
    b el0_a64_irq   
    .align 7
    b el0_a64_fiq
    .align 7
    b el0_a64_serror

    .align 7
    b el0_a32_sync
    .align 7
    b el0_a32_irq
    .align 7
    b el0_a32_fiq
    .align 7
    b el0_a32_serror

// 이거 어디서 쓰는지 확인해야하는데
new_context:
    mov sp, x0               

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