#ifndef KERNEL_KSH_INTERNAL_H
#define KERNEL_KSH_INTERNAL_H

#include "frm/ksh.h"
#include "manage/_fm.h"

extern stack dir_path;

enum shell_ret
{
    COMM_OK = 0,
    COMM_ERR,

    UNKNOW_ERR,
    GRMMER_ERR,

    ARGS_ERR,
};

uint16_t kshputs_error(const char *s);
void fput_uint(uint64_t n);

uint8_t file_split(char **args, fcb_e *current_dir, uint8_t depth);
uint8_t mem_split(char **args);
uint8_t proc_split(char **args);

#endif
