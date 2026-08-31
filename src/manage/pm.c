#include "global/_io.h"
#include "handler/_sync.h"
#include "manage/_mm.h"
#include "global/_debug.h"

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
pcb_t *pm_creat(PMv1_object *obj, uint64_t entry, uint8_t parid)
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
    return pm_creat(obj, (uint64_t)task, parid);
}