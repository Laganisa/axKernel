.global el0_a32_sync
.global el0_a32_irq
.global el0_a32_fiq
.global el0_a32_serror

el0_a32_sync:
    bl check_inf_loop
    b hang

el0_a32_irq:
    bl check_inf_loop
    b hang

el0_a32_fiq:
    bl check_inf_loop
    b hang

el0_a32_serror:
    bl check_inf_loop
    b hang

hang:
    b hang
    