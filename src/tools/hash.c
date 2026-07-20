#include "tools/_hash.h"

#define FNV_OFFSET_BASIS_64 0xcbf29ce484222325ULL
#define FNV_PRIME_64 0x100000001b3ULL

uint64_t fnv1a_hash_64(const char *str)
{
    uint64_t hash = FNV_OFFSET_BASIS_64;

    while (*str)
    {
        hash ^= (uint64_t)(uint8_t)(*str);
        hash *= FNV_PRIME_64;
        str++;
    }

    return hash;
}

uint64_t djb2_hash_64(const char *str)
{
    uint64_t hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}