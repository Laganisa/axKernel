.global el0_a64_sync
.global el0_a64_irq
.global el0_a64_fiq
.global el0_a64_serror

.extern current_task_id

el0_a64_sync:
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

el0_a64_sync_p2:

    // 인터럽트 제거하기
    msr daifset, #2
    
    // 1. 유저의 x0 백업본(스택 #248)은 그대로 두고, 
    //    시스템 콜 리턴값(x0)을 스택의 #0 위치(x0 복원 위치)에 넣습니다.
    str x0, [sp, #0]
    
    // 2. 모든 범용 레지스터 복구
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

    // 3. PC와 SPSR 복원
    ldp x1, x2, [sp, #256]
    msr elr_el1, x1
    msr spsr_el1, x2

    // 4. 스택 정리 및 복귀
    add sp,sp, #272

    // 인터럽트 허용
    msr daifclr, #2
    
    eret

el0_a64_irq:
    // 1. 현재 프로세스(PCB) 주소 로드
    ldr x9, =current_proc
    ldr x10, [x9]             // x10 = pcb_t * (베이스 포인터로 사용)

    // 2. x0~x29 저장 (x10은 PCB 포인터이므로 저장 시 주의)
    // x10이 PCB 포인터이므로 [x10]에 접근하는 동안 x10 값을 보존해야 함
    stp x0,  x1,  [x10, #(0 * 8)]
    stp x2,  x3,  [x10, #(2 * 8)]
    stp x4,  x5,  [x10, #(4 * 8)]
    stp x6,  x7,  [x10, #(6 * 8)]
    stp x8,  x9,  [x10, #(8 * 8)]
    // x10 레지스터 자체를 저장해야 하므로, 임시 레지스터(x11)에 
    // PCB 주소를 백업해두고 저장하는 것이 정석입니다.
    mov x11, x10
    stp x10, x12, [x11, #(10 * 8)] 
    stp x13, x14, [x11, #(12 * 8)]
    stp x15, x16, [x11, #(14 * 8)]
    stp x17, x18, [x11, #(16 * 8)]
    stp x19, x20, [x11, #(18 * 8)]
    stp x21, x22, [x11, #(20 * 8)]
    stp x23, x24, [x11, #(22 * 8)]
    stp x25, x26, [x11, #(24 * 8)]
    stp x27, x28, [x11, #(26 * 8)]
    stp x29, x30, [x11, #(28 * 8)] // x30(LR)도 포함하여 저장

    // 3. 사용자 모드 스택 포인터 (sp_el0) 저장
    mrs x0, sp_el0
    str x0, [x11, #(31 * 8)]

    // 4. SPSR & ELR 저장
    mrs x0, spsr_el1
    str x0, [x11, #(32 * 8)]
    mrs x0, elr_el1
    str x0, [x11, #(33 * 8)]

    // 5. 핸들러 호출 (인자로 현재 PCB 전달)
    mov x0, x11
    bl irq_handler_main

    // 6. 스케줄러가 반환한 다음 프로세스(next_pcb)로 전환
    // x0에 다음 pcb 주소가 담겨있다고 가정
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
    b el0_a64_sync_p2

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