#include "types.h"
#include "io.h"
#include "sync.h"
#include "syscall.h"
#include "pm.h"

#include "debug.h"

// 여기도 수정해야함
extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);
extern void _proc(pcb_t *);

int32_t (*call_table[15])(uint64_t, uint64_t, uint64_t) = {
    [1] = &exit_call,
    [6] = &write_call,
    [7] = &read_call,
    [10] = &open_call};

// Simple syscall handler
uint64_t handle_svc_a64(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    // enter("handle_syscall");

    // reg_x8();

    if (call_table[syscall_num] != NULL)
    {
        int32_t ret = call_table[syscall_num](arg1, arg2, arg3);
        return ret;
    }
    else
    {
        puts("[Kernel] Unknown syscall: ");
        put_hex(syscall_num);
        puts("\n");

        full_stop();

        return -1ULL;
    }
}

int32_t write_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("sys_write");

    // arg1: fd (0=stdin, 1=stdout, 2=stderr)
    // arg2: buffer pointer
    // arg3: length

    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    dump("arg3", arg3);
    */

    if (arg1 == 1 && arg2)
    {

        for (uint64_t i = 0; i < arg3; i++)
        {

            putchar(((int8_t *)arg2)[i]);
        }
    }
    return arg3;
}

int32_t read_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    dump("arg3", arg3);
    */

    int fd = (int)arg1;
    char *buf = (char *)arg2;
    size_t count = (size_t)arg3;

    if (fd != 0)
        return -1;

    if (count == 0)
        return 0;

    char c = getchar();

    putchar(c);

    buf[0] = c;
    return 1;
}

// ! 수정하기
int32_t exit_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("sys_exit");
    // arg1: exit code
    {
        pcb_t *current = get_current_proc_addr();
        pcb_t *next;

        pm_awake(&pm_object, 1, current);
        next = pm_run(&pm_object);
        current_proc = next;

        if (next != 0)
        {
            // ! 바꾸기
            _proc((uint64_t *)next->sp);
        }

        // 대기 함수 이거 나중에 바꿔야지
        while (1)
            ;
    }
    return 0;
}

int32_t open_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    // 인자를 받기
    char *path = (char *)arg1;
    int flags = (int)arg2;
    int mod = (int)arg3;
}