// #include "types.h"
// #include "defs.h"
#include "_io.h"
#include "_sync.h"

#include "_mm.h"
// #include "pm.h"

#include "_debug.h"

extern void _proc(pcb_t *); // proc와 연결

/*
    프로세스 생성 및 삭제와 관련한 파일
*/

/*
    프로세스 생성하는 함수
    프로세스로 만들고 싶어하는 함수의 주소랑
    이 프로세스를 생성한 부모 프로세스의 id 값을 받고
    생성함
*/
pcb_t *creat_proc_entry(PMv1_object *obj, uint64_t entry, uint8_t parid)
{
    enter("creat_proc");
    // id 로직
    uint64_t target_chunk;
    uint64_t leading_zeros;
    uint8_t temp_id;

    uint64_t search_com = ~(uint64_t)obj->proc_comocc;
    asm volatile("clz %0, %1" : "=r"(leading_zeros) : "r"(search_com << 60));

    obj->occ_num = (uint8_t)leading_zeros;

    // if (obj->occ_num >= 4) 이면 나가게
    // 선택된 64비트 청크 내에서 빈자리(0) 찾기
    target_chunk = ~obj->proc_occ[obj->occ_num]; // 0을 1로 반전
    asm volatile("clz %0, %1" : "=r"(leading_zeros) : "r"(target_chunk));

    // PID 계산 (청크 번호 * 64 + 비트 위치)
    uint8_t bit_pos = 63 - (uint8_t)leading_zeros;
    uint8_t pid = (obj->occ_num << 6) | bit_pos;

    // ! 사용한 비트 1로 채우기 (나중에 occ_num 청크가 다 차면 comocc도 1로)
    obj->proc_occ[obj->occ_num] |= (1ULL << bit_pos);
    if (obj->proc_occ[obj->occ_num] == ~0ULL)
    {
        obj->proc_comocc |= (1ULL << (3 - obj->occ_num));
    }

    temp_id = 64 - pid; // 순방향으로 바꾸기

    pcb_t *new_proc = &obj->PMv1_mem[temp_id];

    dump("id", temp_id);
    dump("entry", entry);

    new_proc->id = temp_id;  // 프로세스의 id를 할당된 pid로 변경
    new_proc->b_id = pid;    // 죽을때 쓸 id를 저장
    new_proc->p_id = parid;  // 부모 id를 수정함
    new_proc->proc_info = 0; // 정보를 0으로 수정

    // 메모리 로직
    // 128KB를 할당 리턴 된 메모리 스택 주소를 받음
    new_proc->mm_addr = mm_creat(&mm_stack, INITIAL_PROC_SIZE);

    // 할당 후 주소를 줌
    // 자신의 주소를 알아내고
    uint64_t real_addr = mm_find(&mm_stack, new_proc->mm_addr, 0);

    dump("sp ", real_addr + (INITIAL_PROC_SIZE << 10));

    for (int i = 0; i < 31; i++)
    {
        new_proc->reg_x[i] = 0; // x0~x30 초기화
    }

    new_proc->elr_el1 = entry;                            // (ELR_EL1)
    new_proc->sp = real_addr + (INITIAL_PROC_SIZE << 10); // sp
    new_proc->spsr = (entry == 0) ? 0x3c0 : 0x3c5;        // 인셉션 레벨 분기

    return new_proc;
}

/*
    프로세스 생성 함수로 넘겨주는 래퍼
*/
pcb_t *creat_proc(PMv1_object *obj, void *task, uint8_t parid)
{
    return creat_proc_entry(obj, (uint64_t)task, parid);
}

/*
    proc to proc 전송 함수
    cmd = 0 : 메시지 전송 함수
    cmd = 1 : 메시지 수신 함수
    who가 towho에게 msg를 실행
*/
void ptp(PMv1_object *obj, uint8_t who, uint8_t towho, int8_t msg[64])
{
    pcb_t *rece = &obj->PMv1_mem[towho];
    if (rece->is_msgbox == FALSE)
    {
        // 메시지 넣는 로직
        rece->is_msgbox = TRUE;
        rece->from = who;
        for (int i = 0; i < 64; i++)
        {
            rece->msgbox[i] = msg[i];
        }

        // towho의 우선순위를 증가시켜 바로 입력 받을 수 있도록
        pm_low(obj, 1, towho);  // 대상자를 빼고
        pm_high(obj, 0, towho); // 대상자를 넣고
    }
    else
    {
        // ! 예외 처리
    }
}