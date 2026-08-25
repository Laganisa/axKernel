#ifndef __KERNEL_DTB_H__
#define __KERNEL_DTB_H__

#include "_types.h"

void parse_dtb_tokens(uint64_t dtb_addr);

void parse_dtb(uint64_t dtb_addr);

#endif