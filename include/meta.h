#include "../include/fm.h"

#ifndef __META_H__
#define __META_H__

pcb_t *proc_turn(FMv2_record *reco, int8_t *name, void *entry_point, uint8_t mod);

pcb_t *schedule_proc(pcb_t *proc, uint64_t current_sp);

#endif