.section ".text.boot"
.global _start

_start:
    mrs x0, CurrentEL
    cmp x0, #0xC            // EL3
    beq el3_entry
    cmp x0, #0x8            // EL2
    beq el2_entry
    b el1_entry

// EL3 to EL2
el3_entry:
    // 비 보안 세상으로 전환
    mrs x0, SCR_EL3
    orr x0, x0, #(1 << 0)       // NS = 1
    orr x0, x0, #(1 << 8)
    orr x0, x0, #(1 << 10)
    msr SCR_EL3, x0

    // EL2 진입 시 상태
    mov x0, #0x3C9              // EL2h, IRQ/FIQ/SError mask
    msr SPSR_EL3, x0

    adr x0, el2_entry
    msr ELR_EL3, x0

    eret

    // EL1은 AArch64로 두고, 가상화 기능은 켜지 않는다.
    // 인터럽트는 EL1 커널에서 처리한다.

el2_entry:

    // EL1은 AArch64로 두고, 가상화 기능은 끄고
    // 인터럽트는 EL1 커널에서 처리하자

    mov x0, #0x303              // 0b001100000011
    msr CNTHCTL_EL2, x0

    mov x0, #0
    msr CNTVOFF_EL2, x0

    mov x0, #(1 << 31)

    msr HCR_EL2, x0

    mov x0, #0x3C5              // EL1h
    msr SPSR_EL2, x0

    adr x0, el1_entry
    msr ELR_EL2, x0

    eret

// EL1 KERNEL ENTRY
el1_entry:
    // 스택 설정
    // = stack_top으로 바꾸기
    ldr x0, =stack_top
    mov sp, x0

    msr DAIFClr, #0xf

    // VBAR 초기화
    adr x0, vector_table
    msr VBAR_EL1, x0

    isb
    
    ldr x0, =bss_start
    ldr x1, =bss_end
    mov x2, #0

clear_bss:
    cmp x0, x1
    b.ge clear_bss_done
    str x2, [x0], #8
    b clear_bss

clear_bss_done:
    bl master

hang:
    b hang