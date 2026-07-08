.global el1_sp0_sync
.global el1_sp0_irq
.global el1_sp0_fiq
.global el1_sp0_serror

el1_sp0_sync:
    bl check_inf_loop
    b hang

el1_sp0_irq:
    bl check_inf_loop
    b hang

el1_sp0_fiq:
    bl check_inf_loop
    b hang

el1_sp0_serror:
    bl check_inf_loop
    b hang

hang:
    b hang