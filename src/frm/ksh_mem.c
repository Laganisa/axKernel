#include "frm/ksh.h"
#include "ksh_internal.h"

uint8_t mem_split(char **args)
{
    kshputs_error("sorry yet devo\n");
    return 0;

    /*
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    for (int i = 0; i < 6; i++)
    {
        targs[i] = str_to_uint(args[i]);
    }

    switch (hash_check)
    {

    default:
        kshputs_error("Syntax : mem {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
    */
}
