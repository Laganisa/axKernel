#ifndef __HASH_H__
#define __HASH_H__

#include "_types.h"

#define FNV_OFFSET_BASIS_64 0xcbf29ce484222325ULL
#define FNV_PRIME_64 0x100000001b3ULL

// 간단한 헤시 함수
uint64_t fnv1a_hash_64(const char *str);
uint64_t djb2_hash_64(const char *str);

#endif