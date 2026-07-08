ENTRY(_start)

SECTIONS
{
    /* 커널 로드 주소 (RAM 시작 지점) */
    . = 0x40000000;
    
    .text : {
        KEEP(*(.text.boot))
        *(.text.proc*)       
        *(.text)
        *(.text.*)           
    }
    
    . = ALIGN(8);            
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