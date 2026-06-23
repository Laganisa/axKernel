/* 오류없는지 확인 필요 */
ENTRY(_start)

SECTIONS
{
    /* 커널 로드 주소 (RAM 시작 지점) */
    . = 0x40000000;
    
    .text : {
        KEEP(*(.text.boot))  /* 부트 코드는 무조건 맨 앞에 유지 */
        *(.text.proc*)        /* 프로세스 코드*/
        *(.text)
        *(.text.*)           
    }
    
    . = ALIGN(8);            /* 8바이트 정렬 (64비트 최적화) */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    . = ALIGN(8);
    .data : {
        *(.data)
        *(.data.*)
    }
    
    . = ALIGN(8);
    .bss : {
        bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        bss_end = .;
    }
    
    . = ALIGN(16);
    .stack : {
        . += 0x8000;
        stack_top = .;
    }
}