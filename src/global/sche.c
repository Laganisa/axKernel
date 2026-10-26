#include "global/_meta.h"
#include "manage/_mm.h"
#include "global/_debug.h"
#include "tools/_asm.h"

extern pcb_t *current_proc;

pcb_t *schedule_proc(pcb_t *proc)
{

    // 다음 프로세스를 큐에서 꺼냄
    pcb_t *next = pm_run(pm_object);

    // 현재 proc를 넣고
    pm_object->lowqueue.push(&(pm_object->lowqueue), proc->id);

    mm_free(&mm_stack, &mm_substack, proc->mm_addr);
    current_proc = pm_run(pm_object); // 다른걸 꺼내자

    return next;
}