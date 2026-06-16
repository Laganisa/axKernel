.align 11 // 테이블 시작 주소 2048바이트 정렬
.global vector_table
.global lower_el_aarch64_irq
.global new_context

.extern current_task_id

vector_table:
    // 현재 EL에서 SP_EL0 사용(거의 사용하지 않음)
    .align 7
    b curr_el_sp0_sync
    .align 7
    b curr_el_sp0_irq
    .align 7
    b curr_el_sp0_fiq
    .align 7
    b curr_el_sp0_serror

    //현재 EL에서 SP_ELx 사용 (커널 내부 예외)
    .align 7
    b curr_el_spx_sync
    .align 7
    b curr_el_spx_irq   // 커널 실행 중 인터럽트 터지면 이 함수 실행
    .align 7
    b curr_el_spx_fiq
    .align 7
    b curr_el_spx_serror

    //하위 EL에서 AArch64 사용 (태스크가 돌다 터질 때)
    .align 7
    b lower_el_aarch64_sync  // 시스템 콜 (svc)
    .align 7
    b lower_el_aarch64_irq   // 태스크 실행 중 타이머 터지면 이 함수 실행
    .align 7
    b lower_el_aarch64_fiq
    .align 7
    b lower_el_aarch64_serror

    // 하위 EL에서 AArch32 사용 (거의 안 씀)
    .align 7
    b lower_el_aarch32_sync
    .align 7
    b lower_el_aarch32_irq
    .align 7
    b lower_el_aarch32_fiq
    .align 7
    b lower_el_aarch32_serror

lower_el_aarch64_irq:
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

    mov x19, sp
    bl get_current_proc_addr
    ldr x20, [x0]
    cbz x20, no_current_proc
    ldr x21, [x20, #80]
    mov x22, x19

    ldp x0, x1, [x19, #0]
    stp x0, x1, [x21, #0]
    ldp x2, x3, [x19, #16]
    stp x2, x3, [x21, #16]
    ldp x4, x5, [x19, #32]
    stp x4, x5, [x21, #32]
    ldp x6, x7, [x19, #48]
    stp x6, x7, [x21, #48]
    ldp x8, x9, [x19, #64]
    stp x8, x9, [x21, #64]
    ldp x10, x11, [x19, #80]
    stp x10, x11, [x21, #80]
    ldp x12, x13, [x19, #96]
    stp x12, x13, [x21, #96]
    ldp x14, x15, [x19, #112]
    stp x14, x15, [x21, #112]
    ldp x16, x17, [x19, #128]
    stp x16, x17, [x21, #128]
    ldp x18, x23, [x19, #144]
    stp x18, x23, [x21, #144]
    ldp x24, x25, [x19, #160]
    stp x24, x25, [x21, #160]
    ldp x26, x27, [x19, #176]
    stp x26, x27, [x21, #176]
    ldp x28, x29, [x19, #192]
    stp x28, x29, [x21, #192]
    ldp x30, x23, [x19, #208]
    stp x30, x23, [x21, #208]
    ldr x23, [x19, #224]
    str x23, [x21, #224]
    ldr x23, [x19, #240]
    str x23, [x21, #240]
    ldp x0, x1, [x19, #256]
    stp x0, x1, [x21, #256]

    mov x0, x20
    mov x1, x19
    bl irq_handler_main

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

curr_el_spx_irq:

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

    mov x2, x0
    ldr x0, [x2]             
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