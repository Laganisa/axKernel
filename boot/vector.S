.align 11 // 테이블 시작 주소 2048바이트 정렬
.global vector_table
.global new_context

.extern current_task_id // 나중에 주석 달아서 없에기

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
