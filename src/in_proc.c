#include "in_proc.h"
#include "io.h"

/*
    pid 0 : 루트 프로세스
    pid 1 의 부모
*/
void ROOT(void)
{
    // 루트 프로세스
}

/*
    pid 1 :init 프로세스
    데몬들의 부모이자 모든 프로세스의 부모
*/
void INIT(void)
{
    asm volatile("msr daifclr, #2");
}

/*
    임시로 자리를 맡은 프로세스
*/
void temp_posi(void)
{
    puts("\ntemp position\n");

    asm volatile(
        "mov x8, #1\n"
        "svc #0\n" ::: "x8");

    while (1)
    {
        puts("Error\n");
    }
}

#pragma region test_task

void task_void()
{

    asm volatile("msr daifclr, #2");

    while (1)
    {
        puts("B");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_inf_A()
{

    asm volatile("msr daifclr, #2");

    while (1)
    {
        puts("A");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_inf_B()
{

    asm volatile("msr daifclr, #2");

    while (1)
    {
        puts("B");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_stop_B()
{

    asm volatile("msr daifclr, #2");

    int i = 10;
    while (i > 0)
    {
        puts("B");
        i--;
    }

    asm volatile(
        "mov x8, #1\n"
        "svc #0\n" ::: "x8");

    while (1)
    {
        puts("Error: task should be dead!\n");
    }
}

#pragma endregion