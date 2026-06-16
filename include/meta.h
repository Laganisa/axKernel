#include "../include/types.h"
#include "../include/pm.h"
#include "../include/fm.h"

#ifndef __META_H__
#define __META_H__

pcb_t *proc_turn(FMv2_record *reco, int8_t *name, void *entry_point, uint8_t mod);

#endif