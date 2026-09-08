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

/*
    시스템 콜을 연결하는 파일
*/

#pragma region general_call

// 프로세스가 정상종료 시 호출하는 시스템 콜
static uint64_t exit_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
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

// 프로세스가 비정상 종료시 호출하는 시스템 콜
static uint64_t abort_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
}

// 힙에 공간을 할당 받을 때 사용하는 시스템 콜
static uint64_t brk_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
}

// 자신이 프로세스 점유를 남에게 빌려주는 시스템 콜
static uint64_t yield_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
}

// 프로세스와 커널 간 초기 설정
static uint64_t setup_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    enter("setup");
    uint8_t *addr = (uint8_t *)arg1;
    uint8_t rule = (uint8_t)arg2;
    pm_object.proto_arr[current_proc->id].rule = rule;
    pm_object.proto_arr[current_proc->id].addr = addr;
    exit("setup");
    return 1;
}

static uint64_t write_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    // enter("sys_write");

    int fd = (int)arg1;
    void *buf = (void *)arg2;
    uint32_t len = (uint32_t)arg3;

    // 장치에 쓰기
    if (current_proc->control[fd].is_file == 0)
    {
        // "fd 값이 0이다"라는 소리는 uart
        if (fd == 0)
        {
            return current_proc->control[fd].use_dev->write(buf, len);
        }
        return 1;
    }
    // 파일에 쓰기
    else
    {
        uint32_t written = fm_write(
            fm_record,
            current_proc->control[fd].use_file,
            buf,
            len,
            current_proc->control[fd].file_offset);
        current_proc->control[fd].file_offset += written;
        return (int32_t)written;
    }
}

static uint64_t read_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    // ! 근데 이거 길이 입력 방식이 필요할 듯
    // TODO:
    int fd = (int)arg1;
    char *buf = (char *)arg2;
    size_t count = (size_t)arg3;
    uint32_t offset = (uint32_t)arg4;

    if (count == 0)
    {
        return 0;
    }

    // 장치 읽기일 경우
    if (current_proc->control[fd].is_file == 0)
    {
        char c = getchar();

        putchar(c);

        buf[0] = c;
        return 1;
    }
    // 파일 읽기일 경우
    else
    {

        if (offset >= current_proc->control[fd].file_offset)
        {
            offset = current_proc->control[fd].file_offset;
        }

        uint32_t read_bytes = fm_read(
            fm_record,
            current_proc->control[fd].use_file,
            (void *)buf,
            arg3,
            offset);

        if (read_bytes > 0)
        {
            current_proc->control[fd].file_offset += 1;
            return 0;
        }

        return -1;
    }
}

#pragma endregion

#pragma region file_call

static uint64_t open_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    */

    // 어디를 어떻게 열지
    char *path = (char *)arg1;
    uint8_t flags = (uint8_t)arg2;

    int now_fd = -1;

    for (int i = 1; i < MAX_CONTROL_NUM; i++)
    {
        if (current_proc->control[i].is_ctrl_alloc == 0)
        {
            current_proc->control[i].is_ctrl_alloc = 1;
            now_fd = i;
            break;
        }
    }

    if (now_fd == -1)
    {

        return -1;
    }

    // Device
    if ((flags & 1) != 0)
    {

        dcb_t *dev = dm_find(dm_driver, path);

        if (dev == NULL)
        {
            current_proc->control[now_fd].is_ctrl_alloc = 0;
            return -1;
        }

        current_proc->control[now_fd].use_dev = dev;
        current_proc->control[now_fd].is_file = FALSE;
        current_proc->control[now_fd].file_offset = 0;

        return now_fd;
    }
    // File
    else
    {
        fcb_t *fil = fm_find(fm_record, path);

        if (fil == NULL)
        {
            current_proc->control[now_fd].is_ctrl_alloc = 0;
            return -1;
        }

        current_proc->control[now_fd].use_file = fil;
        current_proc->control[now_fd].is_file = TRUE;

        // append
        if (((flags >> 3) & 1) != 0)
        {
            current_proc->control[now_fd].file_offset = fil->lens * 1024;
        }
        else
        {
            current_proc->control[now_fd].file_offset = 0;
        }

        return now_fd;
    }
}

static uint64_t close_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    int fd = (int)arg1;

    if (fd == 0)
    {
        current_proc->control[fd].use_dev = &uart_device;
    }
    else
    {
        current_proc->control[fd].is_ctrl_alloc = 0;
        current_proc->control[fd].use_dev = NULL;
    }

    return 1;
}

static uint64_t file_creat_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
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

static uint64_t file_del_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
        dump("arg3", arg3);
    */
    // TODO:
}

static uint64_t dir_creat_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
        dump("arg3", arg3);
    */
    // TODO:
}

static uint64_t dir_del_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
        dump("arg3", arg3);
    */
    // TODO:
}

static uint64_t disk_load_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
        dump("arg3", arg3);
    */
    // TODO:
}

static uint64_t disk_store_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        dump("arg1", arg1);
        dump("arg2", arg2);
        dump("arg3", arg3);
    */
    // TODO:
}

#pragma endregion

#pragma region proc_call

// 프로세스 생성 함수
static uint64_t proc_creat_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    dump("arg3", arg3);
    */
    // TODO:
}

// 프로세스 제거 함수
static uint64_t proc_del_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
    dump("arg1", arg1);
    dump("arg2", arg2);
    dump("arg3", arg3);
    */
    // TODO:
}

#pragma endregion

#pragma region ipc_call

static uint64_t ipc_send_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    uint8_t *data = (uint8_t *)arg1;
    uint8_t len = (uint8_t)arg2;
    uint8_t towho = (uint8_t)arg3;

    uint8_t who = current_proc->id;

    ptp(&pm_object, who, towho, data, len);
    *(pm_object.proto_arr[towho].addr) = 3;
    return 1;
}

static uint64_t ipc_rece_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    enter("ipc");

    uint8_t *addr = (uint8_t *)arg1;

    dump("addr", (uint64_t)addr);
    dump("before", addr[0]);

    for (int i = 0; i < current_proc->msgs.len; i++)
    {
        addr[i] = current_proc->msgs.msgbox[i];
    }

    dump("after0", addr[0]);
    dump("after1", addr[1]);

    exit("ipc");

    return 1;
}

#pragma endregion

#pragma region L2toL3

static uint64_t l2_send_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
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

    uint8_t *dst = nm_connect.dst_buf[id];

    if (dst == NULL || nm_connect.is_dst[id] == 0)
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
static uint64_t l2_rece_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        일단 스캘레톤으로 만듬
        아니 인자 왜 받는거임?
        TODO: 검증이 필요
    */
    char *data = (char *)arg1;
    uint8_t *dst = (uint8_t *)arg2;
    uint16_t type = (uint16_t)arg3;

    // 온 순서대로 리턴하기
    enter("rece_L2_call");
    uint8_t ret = nm_connect.nmqueue.pop(&(nm_connect.nmqueue));
    return nm_connect.payload_buf[ret];
}

static uint64_t l2_find_call(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    /*
        ARP 요청 보네고 인덱스를 리턴하기
    */
    // TODO:
    char *path = (char *)arg1;
    int mode = (int)arg2;
    uint32_t size = (uint32_t)arg3;
}

#pragma endregion

static uint64_t (*call_table[40])(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) = {
    /* General */
    [SYS_EXIT] = exit_call,
    [SYS_ABORT] = abort_call,
    [SYS_BRK] = brk_call,
    [SYS_YIELD] = yield_call,
    [SYS_SETUP] = setup_call,
    [SYS_WRITE] = write_call,
    [SYS_READ] = read_call,

    /* File */
    [SYS_OPEN] = open_call,
    [SYS_CLOSE] = close_call,
    [SYS_FILE_CREAT] = file_creat_call,
    [SYS_FILE_DEL] = file_del_call,
    [SYS_DIR_CREAT] = dir_creat_call,
    [SYS_DIR_DEL] = dir_del_call,
    [SYS_DISK_LOAD] = disk_load_call,
    [SYS_DISK_STORE] = disk_store_call,

    /*IPC*/
    [SYS_IPC_SEND] = ipc_send_call,
    [SYS_IPC_RECE] = ipc_rece_call,

    /* Process */
    [SYS_PROC_CREAT] = proc_creat_call,
    [SYS_PROC_DEL] = proc_del_call,

    /* Network */
    [SYS_L2_SEND] = l2_send_call,
    [SYS_L2_RECE] = l2_rece_call,
    [SYS_L2_FIND] = l2_find_call};

uint64_t svc_a64_handle(
    uint64_t syscall_num,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{

    if (call_table[syscall_num] != NULL)
    {
        uint64_t ret = call_table[syscall_num](arg1, arg2, arg3, arg4, arg5);
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
