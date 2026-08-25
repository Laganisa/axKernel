.section ".text.boot"
.global _start

_start:
    mov x3, x0 // DTB 파싱을 위한 처리
    cbz x0, dtb_fallback
    b el1_entry

dtb_fallback:
    ldr x3, =0x40000000
    b el1_entry
    
// EL1 KERNEL ENTRY
el1_entry:
    // 스택 설정
    ldr x0, =stack_top
    mov sp, x0

    msr DAIFClr, #0xf

    // VBAR 초기화
    adr x0, vector_table
    msr VBAR_EL1, x0

    isb // 파이프라인 동기화
    
    ldr x0, =bss_start
    ldr x1, =bss_end
    mov x2, #0

clear_bss:
    cmp x0, x1
    b.ge bl_main
    str x2, [x0], #8
    b clear_bss

bl_main:
    mov x0, x3 // DTB 처리를 위해 커널에 인자로 넘겨주기
    bl master

hang:
    b hang