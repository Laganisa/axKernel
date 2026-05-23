.section ".text.proc"
.global _proc

_proc:
    // 코드
    // 1. PC (reg_val[32]) to ELR_EL1 (Exception Link Register)
    // cpu 점프 주소
    ldr x1, [x0, #(32 * 8)]
    msr elr_el1, x1

    // 2. SP (reg_val[31]) to SP_EL0
    // 프로세스가 사용할 전용 스택 포인터
    ldr x1, [x0, #(31 * 8)]
    mov sp, x1

    // 3. SPSR_EL1 (Processor State) 복원
    // reg_val[33]에 저장된 모드 값을 사용한다.
    ldr x1, [x0, #(33 * 8)]
    msr spsr_el1, x1

    // 4. 일반 목적 레지스터 복원 (x1 ~ x30)
    ldp x1, x2, [x0, #(1 * 8)]
    ldp x3, x4, [x0, #(3 * 8)]
    ldp x5, x6, [x0, #(5 * 8)]
    ldp x7, x8, [x0, #(7 * 8)]
    ldp x9, x10, [x0, #(9 * 8)]
    ldp x11, x12, [x0, #(11 * 8)]
    ldp x13, x14, [x0, #(13 * 8)]
    ldp x15, x16, [x0, #(15 * 8)]
    ldp x17, x18, [x0, #(17 * 8)]
    ldp x19, x20, [x0, #(19 * 8)]
    ldp x21, x22, [x0, #(21 * 8)]
    ldp x23, x24, [x0, #(23 * 8)]
    ldp x25, x26, [x0, #(25 * 8)]
    ldp x27, x28, [x0, #(27 * 8)]
    ldp x29, x30, [x0, #(29 * 8)]

    ldr x0, [x0]

    // CPU가 ELR_EL1에 적힌 주소로 이동
    eret
