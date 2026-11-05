#include "manage/_gm.h"
#include "global/_debug.h"
#include "global/_io.h"

void wm_init()
{
}

wcb_t wm_create(
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h,
    void *func)
{
    wcb_t new_wcb;

    new_wcb.x = x;
    new_wcb.y = y;
    new_wcb.w = w;
    new_wcb.h = h;

    new_wcb.draw = (uint64_t)func;

    return new_wcb;
}

void wm_delate()
{
}