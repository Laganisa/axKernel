#include "io.h"
#include "irq.h"
#include "exce.h"
#include "syscall.h"

#include "debug.h"

// Called from exception path when a data abort from lower EL occurs.
// Logs FAR/ELR/ESR and terminates the current process, scheduling next.
// ! 이거 나중에 어셈블리로 옮길 예정
void handle_data_abort(uint64_t far, uint64_t elr, uint64_t esr)
{

    reg_far_el1();
    reg_elr_el1();

    pcb_t *current = get_current_proc_addr();
    pcb_t *next;

    pm_awake(&pm_object, 1, current);
    next = pm_run(&pm_object);
    current_proc = next;

    if (next != 0)
    {
        new_context(next->sp);
    }

    full_stop();
}
