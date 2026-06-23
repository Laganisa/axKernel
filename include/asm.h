#include "../include/types.h"
#include "fm.h"

#ifndef __ASM_H__
#define __ASM_H__

// 입력된 값의 1의 개수를 카운트
static inline uint8_t cnt(uint64_t val)
{
    // Hamming Weight 알고리즘 (64비트용)
    val = val - ((val >> 1) & 0x5555555555555555ULL);
    val = (val & 0x3333333333333333ULL) + ((val >> 2) & 0x3333333333333333ULL);
    return (uint8_t)((((val + (val >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL) >> 56);
}

static inline uint64_t ubfx(uint64_t source, uint32_t lsb, uint32_t width)
{
    uint64_t result;
    uint64_t mask = (1ULL << width) - 1;
    asm volatile(
        "lsr %0, %1, %2\n\t" // source를 lsb만큼 오른쪽으로 밀고
        "and %0, %0, %3"     // mask랑 AND 해서 원하는 폭만 남기기
        : "=r"(result)
        : "r"(source), "r"((uint64_t)lsb), "r"(mask));
    return result;
}

// 시프트랑 더하기 동시에 하기
static inline uint64_t add_lsl(uint64_t base, uint64_t offset, uint32_t shift)
{
    uint64_t result;
    asm volatile(
        "add %0, %1, %2, lsl %3"
        : "=r"(result)
        : "r"(base), "r"(offset), "i"(shift));
    return result;
}

// clz을 세주는 함수 1이 나오기까지 0의 수를 리턴
static inline uint32_t clz(uint64_t val)
{
    uint32_t res;
    __asm__ volatile(
        "clz %0, %1"
        : "=r"(res)
        : "r"(val));
    return res;
}

#pragma region not_asm

static inline void memcpy(uint8_t *dst, uint8_t *src, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

static inline void normalize_path(const int8_t *src, int8_t dst[27])
{
    uint8_t i = 0;

    while (i < 26 && src[i] != '\0')
    {
        dst[i] = src[i];
        i++;
    }

    while (i < 26)
    {
        dst[i++] = 0x20;
    }

    dst[26] = '\0';
}

static inline uint32_t slot_index(fcb_t *file)
{
    if (file->depth == 0)
    {
        return (uint32_t)file->ppdir_addr;
    }

    if (file->depth == 1)
    {
        return 16U + ((uint32_t)file->ppdir_addr * 16U) + (uint32_t)file->pdir_addr;
    }

    return 16U + 256U + ((uint32_t)file->ppdir_addr * 256U) + ((uint32_t)file->pdir_addr * 16U) + (uint32_t)file->me_addr;
}

#pragma endregion

#endif