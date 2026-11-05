ENTRY(_start)

SECTIONS
{
    /* 커널 로드 주소 (RAM 시작 지점) */
    . = 0x40080000;
    
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
        . = ALIGN(16);
        __kernel_bss_start = .;
        __mm_addr_start = .;
        . += 0x6000;
        __mm_addr_end = .;
        __pm_addr_start = .;
        . += 0x1F000;
        __pm_addr_end = .;
        __fm_addr_start = .;
        . += 0x50000;
        __fm_addr_end = .;
        __dm_addr_start = .;
        . += 0x5000;
        __dm_addr_end = .;
        __nm_addr_start = .;
        . += 0x5000;
        __nm_addr_end = .;
        __gm_addr_start = .;
        . += 0x5000;
        __gm_addr_end = .;
        __kernel_data_end = .;
        __kernel_bss_end = .;
        bss_end = .;
    }
    
    . = ALIGN(16);
    __kernel_stack_start = .;
    .stack : {
        . += 0x10000;
        stack_top = .;
    }

    __kernel_stack_end = .;
    __kernel_heap_start = ALIGN(16, .);
    . = __kernel_heap_start + 0x80000;
    __kernel_heap_end = .;

    __user_proc_start = ALIGN(4096, .);
    __user_proc_end = __user_proc_start + 0x10000000;
    __user_file_start = __user_proc_end;
    __user_file_end = __user_file_start + 0x88755000;

    __kernel_start = ADDR(.text);
}