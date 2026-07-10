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