#ifndef __KERNEL_TYPES_H__
#define __KERNEL_TYPES_H__

// 기본 데이터 타입 정의
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef uint64_t size_t;
typedef int64_t ssize_t;

// 포인터 타입
typedef uint64_t uintptr_t;
typedef int64_t intptr_t;

// 여기서 쓸 타입들
typedef uint8_t bool;

// NULL 정의
#define NULL ((void *)0)

// TRUE / FALSE
#define TRUE 1
#define FALSE 0

#endif // __TYPES_H__
