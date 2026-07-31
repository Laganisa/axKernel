#include "_types.h"
#include "global/_io.h"
#include "handler/_sync.h"
#include "handler/_syscall.h"
#include "manage/_pm.h"
#include "manage/_dm.h"
#include "global/_debug.h"
#include "manage/_nm.h"

#include "tools/_virtio.h"

extern pcb_t *current_proc;
extern pcb_t *get_current_proc_addr(void);
extern void _proc(pcb_t *);

extern dcb_t uart_device;

int32_t (*call_table[40])(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) = {
    /* General */
    [SYS_EXIT] = exit_call,
    /*[SYS_ABORT] = abort_call,
    [SYS_LOAD] = load_call,
    [SYS_YIELD] = yield_call,*/
    [SYS_SETUP] = setup_call,
    [SYS_WRITE] = write_call,
    [SYS_READ] = read_call,

    /* File */
    [SYS_FILE_CREAT] = creat_file_call,
    [SYS_FILE_DEL] = del_file_call,
    [SYS_OPEN] = open_call,
    [SYS_CLOSE] = close_call, /*
     [SYS_DIR_CREAT] = creat_dir_call,
     [SYS_DIR_DEL] = del_dir_call,
     */

    /* Process */
    [SYS_PROC_CREAT] = creat_proc_call,
    /*[SYS_PROC_DEL] = del_proc_call,
     */

    /* Network */
    [SYS_SEND_L2] = send_L2_call};

/*
    시스템 콜을 연결하는 파일
*/

uint64_t handle_svc_a64(
    uint64_t syscall_num,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    // reg_x8();

    if (call_table[syscall_num] != NULL)
    {
        int32_t ret = call_table[syscall_num](arg1, arg2, arg3, arg4, arg5);
        return ret;
    }
    else
    {
        puts("[Kernel] Unknown syscall: ");
        put_hex(syscall_num);
        puts("\n");

        full_stop();

        return 0ULL;
    }
}

#pragma region general_call

int32_t setup_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    uint8_t *addr = (uint8_t *)arg1;
    uint8_t rule = (uint8_t)arg2;
    pm_object.proto_arr[current_proc->id].rule = rule;
    pm_object.proto_arr[current_proc->id].addr = addr;
    return 1;
}

int32_t write_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
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

    // 장치에 쓰기
    if (current_proc->is_file == 0)
    {
        if (arg1 == 1 || arg1 == 2)
        {
            if (arg1 == 2)
            {
                puts("[debug]");
            }
            return current_proc->use_dev->write(arg2, arg3);
        }
        return 1;
    }
    // 파일에 쓰기
    else
    {
        uint32_t written = fm_write(
            fm_record,
            current_proc->use_file,
            (void *)arg2,
            (uint32_t)arg3,
            current_proc->file_offset);
        current_proc->file_offset += written;
        return (int32_t)written;
    }
}

int32_t read_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{

    int fd = (int)arg1;
    char *buf = (char *)arg2;
    size_t count = (size_t)arg3;

    if (count == 0)
    {
        return 0;
    }

    // 장치 읽기일 경우

    if (current_proc->is_file == 0)
    {
        char c = getchar();

        putchar(c);

        buf[0] = c;
        return 1;
    }
    // 파일 읽기일 경우
    else
    {
        uint32_t read_bytes = fm_read(fm_record, current_proc->use_file, (void *)buf, 1, current_proc->file_offset);

        if (read_bytes > 0)
        {
            current_proc->file_offset += 1;
            return 0;
        }
        return -1;
    }
}

int32_t open_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    */

    // 어디를 어떻게 열지
    char *path = (char *)arg1;
    uint8_t flags = (uint8_t)arg2;

    // 플레그의 하위 1비트의 값이 0이면 장치라고 생각
    if ((flags & 1) == 0)
    {

        // 장치를 바꿔주기
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
        fcb_t *fil = fm_find(fm_record, path);

        if (fil == NULL)
        {
            return 0;
        }

        current_proc->use_file = fil;
        current_proc->is_file = TRUE; // 파일을 열었다고 설정

        // 오프셋 설정

        // 새로 작업
        if ((flags >> 3) & 1 != 0)
        {
            current_proc->file_offset = 0;
        }

        return 1;
    }
}

int32_t close_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    current_proc->use_dev = &uart_device;
    current_proc->is_file = FALSE;
    return 1;
}

int32_t exit_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    enter("sys_exit");
    // arg1: exit code
    // ! 아직은 그 인자에 대하여 사용하지 않음
    {
        pcb_t *current = get_current_proc_addr();
        pcb_t *next;

        pm_awake(&pm_object, 1, current);
        next = pm_run(&pm_object);

        // 다음 값이 무었인지 확인하기
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

#pragma endregion

#pragma region file_call

int32_t creat_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
    */

    // 어디를 어떤 식으로 만들지
    char *path = (char *)arg1;
    int mode = (int)arg2;
    uint32_t size = (uint32_t)arg3;

    // ? 뭐 별도의 로직이 없는게 허전하긴함
    fm_create(fm_record, path, size, mode);

    return 1;
}

int32_t del_file_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
        dump("arg3", arg3);
    */
}

#pragma endregion

#pragma region proc_call

int32_t creat_proc_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
}

#pragma endregion

#pragma region L2toL3

int32_t send_L2_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /*
        송신 시스템 콜
        버퍼에 있는걸 복사후 전송
    */

    uint8_t *data = (uint8_t *)arg1;
    uint8_t id = (uint8_t)arg2;
    uint8_t len = (uint8_t)arg3;
    uint16_t type = (uint16_t)arg4;

    /*
        id로 찾는 로직
    */

    uint8_t *dst = nm_connect->dst_buf[id];

    if (dst == NULL || nm_connect->is_dst[id] == 0)
    {
        return -1;
    }

    nm_cap(dst, data, len, type);

    int timeout = 10000000;
    while (timeout--)
    {
        if (tx_queue.used->idx != last_tx_used_idx)
        {
            puts("TX SUCCESS\n");
            last_tx_used_idx++;
            return 0;
        }
    }
    return -1; // 타임아웃
}

// 나중에 프로토콜을 통한 소통으로 만들기
/*
    이 시스템 콜이 들어오면
    1. 일단 관리자 구조체에서 온 신호가 있는지 확인한다.
    2. 만약 없다면 이 프로세스를 재우고 나중에 인터럽트로 올때 자신을 깨우라고 한다.
*/
int32_t rece_L2_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /*
        1. nm_connet을 둘러본다
        2. 없으면 타임아웃이 될 때까지 수신 준비
    */

    char *data = (char *)arg1;
    uint8_t *dst = (uint8_t *)arg2;
    uint16_t type = (uint16_t)arg3;

    // nm_discap(dst, data, type);

    return -1; // 타임아웃
}

int32_t find_L2_call(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    /*
        ARP 요청 보네고 인덱스를 리턴하기
    */

    char *path = (char *)arg1;
    int mode = (int)arg2;
    uint32_t size = (uint32_t)arg3;
}

#pragma endregion