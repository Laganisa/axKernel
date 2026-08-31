#include "global/_in_proc.h"
#include "global/_io.h"
#include "tools/_asm.h"

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
    enable_irq();
}

/*
    임시로 자리를 맡은 프로세스
*/
void temp_posi0(void)
{
    puts("\ntemp position 0\n");

    proc_exit();

    while (1)
    {
        puts("Error\n");
    }
}

void temp_posi1(void)
{
    enable_irq();

    puts("\ntemp position 1\n");

    // char msg_arr[64] = "hello";

    // ptp(&pm_object, 2, 1, msg_arr,);

    while (1)
    {
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }

    proc_exit();

    while (1)
    {
        puts("Error\n");
    }
}

void temp_posi2(void)
{
    enable_irq();

    puts("\ntemp position 2\n");
    char msg_arr[64] = {0};

    // pm_object.PMv1_mem[2].msgs.msgbox = msg_arr;
    pm_object.PMv1_mem[2].msgs.is_msgbox = 0;

    while (pm_object.PMv1_mem[2].msgs.is_msgbox == 0)
    {
        asm volatile("wfi");
    }

    puts("Message arrived: ");
    puts(msg_arr);

    pm_object.PMv1_mem[2].msgs.is_msgbox = 0;

    proc_exit();

    while (1)
    {
        puts("Error\n");
    }
}

#pragma region test_task

void task_void()
{
    enable_irq();

    while (1)
    {
        puts("B");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_inf_A()
{

    enable_irq();

    while (1)
    {
        puts("A");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_inf_B()
{
    enable_irq();

    while (1)
    {
        puts("B");
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_wfi()
{
    enable_irq();

    while (1)
    {
        asm volatile("wfi");
    }
}

void task_hang(void)
{
    enable_irq();

    while (1)
    {
        for (volatile int i = 0; i < 1000000; i++)
            ;
    }
}

void task_stop_B()
{
    enable_irq();

    int i = 10;

    while (i > 0)
    {
        puts("B");
        i--;
    }

    proc_exit();

    while (1)
    {
        puts("Error: task should be dead!\n");
    }
}

#pragma endregion