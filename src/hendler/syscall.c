#include "_types.h"
#include "_io.h"
#include "_sync.h"
#include "_syscall.h"
#include "_pm.h"
#include "_dm.h"

#include "_debug.h"

// 여기도 수정해야함
extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);
extern void _proc(pcb_t *);
extern dcb_t uart_device;

int32_t (*call_table[16])(uint64_t, uint64_t, uint64_t) = {
    [1] = &exit_call,
    [6] = &write_call,
    [7] = &read_call,
    [8] = &creat_file_call,
    [10] = &open_call,
    [11] = &close_call};

// Simple syscall handler
uint64_t handle_svc_a64(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
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
    // enter("sys_write");

    // arg1: fd (0=stdin, 1=stdout, 2=stderr)
    // arg2: buffer pointer
    // arg3: length

    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    dump("arg3", arg3);
    */

    if (arg1 == 1 || arg1 == 2)
    {
        if (arg1 == 2)
        {
            puts("[debug]");
        }
        return current_proc->use_dev->write(arg2);
    }
    return arg3;
}

int32_t open_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    */

    // 어디를 어떻게 열지
    char *path = (char *)arg1;
    int flags = (int)arg2;
    // 아직은 arg2 안씀
    // 장치를 바꿔주기

    // 플레그의 하위 1비트의 값이 0이면 장치라고 생각
    if (flags & 1 == 0)
    {
        // ? 여기서도 플레그 쓰겠지 아마도

        dcb_t *dev = dm_find(dm_driver, path);

        if (dev == NULL)
        {
            return 0;
        }
        current_proc->use_dev = dev;
        return 1;
    }
    // 플레그의 하위 1비트의 값이 1이면 장치가 아님
    else
    {
        // 플레그를 사용한 파일 열기
    }
}

int32_t creat_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    */

    // 어디를 어떤 식으로 만들지
    char *path = (char *)arg1;
    int mode = (int)arg2;
    uint32_t size = (uint32_t)arg3;

    fm_create(fm_record, path, size, mode);

    return 1;
}

int32_t close_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    current_proc->use_dev = &uart_device;
    return 1;
}

// ! DS 이식하기
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

    // ! 아직 VDS 로 바꾸지 않음
    /*
    if (fd == 0 && current_proc->use_dev != NULL)
    {
        // 1. 장치에서 1바이트 읽기 (하드웨어 의존적)
        int bytes_read = current_proc->use_dev->read(buf);

        // 2. 에코는 커널의 약속된 stdout(putchar)을 사용!
        if (bytes_read > 0)
        {
            putchar(*(char *)buf);
        }

        return bytes_read;
    }
    return -1;
    */
}

int32_t exit_call(uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    enter("sys_exit");
    // arg1: exit code
    {
        pcb_t *current = get_current_proc_addr();
        pcb_t *next;

        pm_awake(&pm_object, 1, current);
        next = pm_run(&pm_object);

        dump("next", next);

        current_proc = next;

        if (next != 0)
        {
            _proc(next);
        }

        // 대기 함수 이거 나중에 바꿔야지
        while (1)
            ;
    }
    return 0;
}
