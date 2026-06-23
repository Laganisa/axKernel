#include "../include/types.h"
#include "../include/io.h"
#include "../include/exce.h"
#include "../include/syscall.h"
#include "pm.h"

#include "debug.h"

// 여기도 수정해야함
extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);
extern void _proc(uint64_t *reg_val);

// Simple syscall handler
uint64_t handle_syscall(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("handle_syscall");

    reg_x8();

    reg_far_el1();
    reg_elr_el1();
    reg_esr_el1();

    switch (syscall_num)
    {
    // 시스템 쓰기
    case SYS_WRITE:
    {
        enter("sys_write");
        // arg1: fd (0=stdin, 1=stdout, 2=stderr)
        // arg2: buffer pointer
        // arg3: length

        if (arg1 == 1 && arg2)
        {
            for (uint64_t i = 0; i < arg3; i++)
            {
                putchar(((int8_t *)arg2)[i]);
            }
        }
        return arg3;
    }
    // 시스템
    case SYS_YIELD:
    {
        enter("sys_yield");
        // 처리 로직
        pcb_t *current = get_current_proc_addr();
        pcb_t *
            next;

        pm_awake(&pm_object, 1, current);
        next = pm_run(&pm_object);
        current_proc = next;

        if (next != 0)
        {
            // 바꾸기
            _proc((uint64_t *)next->sp);
        }

        return 0;
    }
    // 시스템 읽기
    case SYS_READ:
    {
        enter("sys_read");

        // arg1 : 0
        int fd = (int)arg1;
        char *buf = (char *)arg2;
        size_t count = (size_t)arg3;

        // uart_flush();

        if (fd == 0)
        {
            size_t i = 0;
            // count만큼 루프를 돌며 한 글자씩 가져옴
            while (i + 1 < count)
            {
                char c = getchar();

                // 엔터 입력 시 종료
                if (c == '\r' || c == '\n')
                {
                    buf[i] = '\0';
                    puts("\n");
                    return (long)i;
                }

                putchar(c);
                buf[i++] = c;
            }
            buf[i] = '\0';
            return (long)i;
        }
        return -1;
    }
    // 시스템 종료
    case SYS_EXIT: // 나중에 시스템 콜 sys_exit로 바꾸기
    {
        // arg1: exit code
        {
            pcb_t *current = get_current_proc_addr();
            pcb_t *next;

            pm_awake(&pm_object, 1, current);
            next = pm_run(&pm_object);
            current_proc = next;

            if (next != 0)
            {
                // 바꾸기
                _proc((uint64_t *)next->sp);
            }

            // 대기 함수 이거 나중에 바꿔야지
            while (1)
                ;
        }
        break;
    }
    default:
        puts("[Kernel] Unknown syscall: ");
        put_hex(syscall_num);
        puts("\n");

        // full_stop();

        return -1ULL;
    }

    return 0;
}
