#include "global/_io.h"
#include "handler/_sync.h"
#include "manage/_mm.h"
#include "global/_debug.h"
#include "global/_alloc.h"
#include "manage/temp_mm.h"

extern void _proc(pcb_t *); // proc와 연결
extern dcb_t uart_device;

static uint8_t pm_user_stack_size(
    uint64_t argc,
    const char *const argv[],
    uint64_t *strings_size)
{
    if (argc > (PROC_STACK_RESERVE / sizeof(uint64_t)) - 2 ||
        (argc != 0 && argv == NULL))
    {
        return FALSE;
    }

    uint64_t total = 0;
    for (uint64_t i = 0; i < argc; i++)
    {
        if (argv[i] == NULL)
        {
            return FALSE;
        }

        uint64_t len = 0;
        while (len < PROC_STACK_RESERVE - total && argv[i][len] != '\0')
        {
            len++;
        }
        if (len == PROC_STACK_RESERVE - total)
        {
            return FALSE;
        }
        total += len + 1;
    }

    uint64_t vector_size = (argc + 2) * sizeof(uint64_t);
    if (vector_size + total + 15 > PROC_STACK_RESERVE)
    {
        return FALSE;
    }

    *strings_size = total;
    return TRUE;
}

/*
    프로세스 생성 및 삭제와 관련한 파일
*/
void pm_init()
{
    queue_init(&(pm_object->lowqueue), pm_object->lowbuf, 255);
    queue_init(&(pm_object->highqueue), pm_object->highbuf, 255);
}

/*
    프로세스 생성하는 함수
    프로세스로 만들고 싶어하는 함수의 주소랑
    이 프로세스를 생성한 부모 프로세스의 id 값을 받고
    생성함
*/
pcb_t *pm_create(
    PMv1_object *obj,
    uint64_t entry,
    uint8_t parid,
    uint64_t argc,
    const char *const argv[])
{
    uint64_t strings_size = 0;
    if ((entry != 0 && argc != 0) ||
        (entry == 0 &&
         !pm_user_stack_size(argc, argv, &strings_size)))
    {
        log("Invalid process start arguments");
        return NULL;
    }

    int16_t id = -1;

    for (int i = 0; i < MAX_PCB_SIZE; i++)
    {
        if (obj->is_alloc[i] == 0)
        {
            obj->is_alloc[i] = 1;
            id = i;
            break;
        }
    }

    if (id == -1)
    {
        return NULL;
    }

    id = (uint8_t)id + 1;
    pcb_t *new_proc = &(obj->PMv1_mem[id]);

    // 프로세스 상태
    new_proc->id = id;      // 프로세스의 id를 할당된 pid로 변경
    new_proc->p_id = parid; // 부모 id를 수정함
    new_proc->state = 0;

    new_proc->heap_start = 0;
    new_proc->heap_break = 0;
    new_proc->heap_limit = 0;

    // 프로세스 메시지
    new_proc->msgs.from = 0;
    new_proc->msgs.is_call = 0;
    new_proc->msgs.is_msgbox = 0;
    new_proc->msgs.len = 0;

    // 메시지 배열 초기화
    memset(new_proc->msgs.msgbox, 0, sizeof(new_proc->msgs.msgbox));

    // 프로세스 조종
    new_proc->control[0].is_ctrl_alloc = 1;      // uart로 정해짐
    new_proc->control[0].use_dev = &uart_device; // 정보를 0으로 수정
    new_proc->control[0].file_offset = 0;        // 파일 오프셋
    new_proc->control[0].is_file = 0;            // 파일을 열지 않음

    // 프로세스에 들어갈 페이지 테이블
    // TODO: 프로세스당 페이지 테이블을 만들기
    new_proc->page_i = (page_t *)heap_alloc(sizeof(page_t));
    if (new_proc->page_i == NULL)
    {
        obj->is_alloc[id - 1] = 0;
        dump("Process page table allocation failed", new_proc->id);
        return NULL;
    }

    // 프로세스 메모리를 할당하고 시작 주소를 구함
    uint16_t mm_top_before = mm_stack.sp_top;
    new_proc->mm_addr = mm_creat(&mm_stack, INITIAL_PROC_SIZE);
    if (mm_stack.sp_top == mm_top_before)
    {
        heap_free(new_proc->page_i);
        obj->is_alloc[id - 1] = 0;
        dump("Process memory allocation failed", INITIAL_PROC_SIZE);
        return NULL;
    }
    uint64_t real_addr = mm_find(&mm_stack, new_proc->mm_addr, 0);

    // 페이지 테이블 초기화
    new_proc->page_i->is_full = 0;
    new_proc->page_i->num = 0;
    new_proc->page_i->is_leaf = 0;

    for (int i = 0; i < 512; i++)
    {
        new_proc->page_i->pages[i] = NULL;
    }

    // 프로세스 데이터 넣기
    for (int i = 0; i < 31; i++)
    {
        new_proc->regs.reg_x[i] = 0; // x0~x30 초기화
    }

    new_proc->regs.elr_el1 = entry;                            // (ELR_EL1)
    new_proc->regs.sp = real_addr + (INITIAL_PROC_SIZE << 10); // sp

    if (entry == 0)
    {
        uint64_t process_size = (uint64_t)INITIAL_PROC_SIZE << 10;
        uint64_t vector_size = (argc + 2) * sizeof(uint64_t);
        uint64_t stack_offset =
            (process_size - vector_size - strings_size) & ~15ULL;
        uint64_t *stack = (uint64_t *)(real_addr + stack_offset);
        uint64_t string_offset = stack_offset + vector_size;

        stack[0] = argc;
        for (uint64_t i = 0; i < argc; i++)
        {
            uint64_t len = 0;
            while (argv[i][len] != '\0')
            {
                len++;
            }

            stack[i + 1] = TEMP_MM_USER_BASE + string_offset;
            for (uint64_t j = 0; j <= len; j++)
            {
                ((char *)(real_addr + string_offset))[j] = argv[i][j];
            }
            string_offset += len + 1;
        }
        stack[argc + 1] = 0;
        new_proc->regs.sp = TEMP_MM_USER_BASE + stack_offset;
    }

    // new_proc->regs.spsr = (entry == 0) ? 0x3c0 : 0x3c5;        // 인셉션 레벨 분기
    new_proc->regs.spsr = (entry == 0) ? 0x340 : 0x3c5; // 인셉션 레벨 분기

    dump("elr_el1", entry);

    return new_proc;
}

/*
    프로세스 생성 함수로 넘겨주는 래퍼
*/
pcb_t *creat_proc(PMv1_object *obj, void *task, uint8_t parid)
{
    return pm_create(obj, (uint64_t)task, parid, 0, NULL);
}

// 프로세스 삭제 함수
pcb_t *pm_delete(
    PMv1_object *obj,
    uint64_t entry,
    uint8_t parid)
{
}

// TODO:
// 어떤 타입을 리턴할지 미정
uint64_t mm_page(uint64_t vaddr)
{
    uint64_t addr = vaddr >> 12;
    uint16_t offset = vaddr & 0xFFF;

    // 현재 프로세스에 있는 페이지 테이블을 가져오기
    pcb_t *now_proc = get_current_proc_addr();

    // 이거 sll로 해서 MM stack 쪽엔 root를 두어야지
    page_t *now_page = now_proc->page_i;

    uint64_t temp;

    // L0 -> L1 -> L2 순서로 상위 비트부터 페이지 인덱스를 가져옴

    // L0
    temp = (addr >> 18) & 0x1FF;

    if (now_page == NULL)
    {
        return 0;
    }

    if (now_page->is_leaf)
    {
        return 0;
    }

    now_page = now_page->pages[temp];

    if (now_page == NULL)
    {
        return 0;
    }

    // L1
    temp = (addr >> 9) & 0x1FF;

    if (now_page->is_leaf)
    {
        return 0;
    }

    now_page = now_page->pages[temp];

    if (now_page == NULL)
    {
        return 0;
    }

    // L2
    temp = addr & 0x1FF;

    // 고른 마지막 페이지가 리프가 아니라면?
    // L0 -> L1 -> L2 로 온다음
    if (!(now_page->is_leaf))
    {
        return 0;
    }

    uint64_t result = now_page->frame[temp] + offset;

    return result;
}