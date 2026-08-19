#include "tools/_utils.h"

size_t kstrlen(const char *str)
{
    const char *p = str;

    while (*p)
    {
        p++;
    }

    return (size_t)(p - str);
}