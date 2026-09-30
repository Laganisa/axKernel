#include "global/_io.h"
#include "handler/_sync.h"
#include "manage/_mm.h"
#include "global/_debug.h"
#include "global/_alloc.h"

extern void _proc(pcb_t *); // proc와 연결
extern dcb_t uart_device;

/*
    프로세스 생성 및 삭제와 관련한 파일
*/
void pm_init()
{
    queue_init(&(pm_object.lowqueue), pm_object.lowbuf, 255);
    queue_init(&(pm_object.highqueue), pm_object.highbuf, 255);
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
    uint8_t parid)
{
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
    pcb_t *new_proc = &obj->PMv1_mem[id];

    // 프로세스 상태
    new_proc->id = id;      // 프로세스의 id를 할당된 pid로 변경
    new_proc->p_id = parid; // 부모 id를 수정함
    new_proc->state = 0;
    new_proc->heap_start = 0;
    new_proc->heap_break = 0;
    new_proc->heap_limit = 0;

    // 프로세스 메시지
    new_proc->msgs.from = NULL;
    new_proc->msgs.is_call = NULL;
    new_proc->msgs.is_msgbox = NULL;
    new_proc->msgs.len = NULL;

    // 메시지 배열 초기화
    memset(new_proc->msgs.msgbox, 0, sizeof(new_proc->msgs.msgbox));

    // 프로세스 조종
    new_proc->control[0].is_ctrl_alloc = 1;      // uart로 정해짐
    new_proc->control[0].use_dev = &uart_device; // 정보를 0으로 수정
    new_proc->control[0].file_offset = 0;        // 파일 오프셋
    new_proc->control[0].is_file = 0;            // 파일을 열지 않음

    // 메모리 로직
    // 128KB를 할당 리턴 된 메모리 스택 주소를 받음
    new_proc->mm_addr = mm_creat(&mm_stack, INITIAL_PROC_SIZE);

    // 할당 후 주소를 줌
    // 자신의 주소를 알아내고
    uint64_t real_addr = mm_find(&mm_stack, new_proc->mm_addr, 0);

    // 프로세스에 들어갈 페이지 테이블
    // TODO: 프로세스당 페이지 테이블을 만들기
    new_proc->page_i = (page_t *)heap_alloc(sizeof(page_t));

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

    // new_proc->regs.spsr = (entry == 0) ? 0x3c0 : 0x3c5;        // 인셉션 레벨 분기
    new_proc->regs.spsr = (entry == 0) ? 0x340 : 0x3c5;

    return new_proc;
}

/*
    프로세스 생성 함수로 넘겨주는 래퍼
*/
pcb_t *creat_proc(PMv1_object *obj, void *task, uint8_t parid)
{
    return pm_create(obj, (uint64_t)task, parid);
}

static inline uint64_t aarch64_rev64(uint64_t val)
{
    uint64_t result;
    __asm__ volatile("rev %0, %1" : "=r"(result) : "r"(val));
    return result;
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