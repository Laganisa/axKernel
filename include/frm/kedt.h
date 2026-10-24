#ifndef __KERNEL_KEDT_H__
#define __KERNEL_KEDT_H__

#include "manage/_fm.h"
#include "global/_io.h"

extern uint32_t io_console_line;
extern uint32_t io_console_xpixel;

enum edt_mode
{
    M_MODE = 0, // 이동 모드
    E_MODE = 1, // 편집 모드
};

void kedt(fcb_t *file);

#endif