#include "../include/types.h"
#include "../include/defs.h"
#include "../include/io.h"
#include "../include/exce.h"

#include "../include/mm.h"
#include "../include/pm.h"

extern void _proc(uint64_t *reg_val); // proc와 연결

/*
    프로세스 생성
    pid 생성을 오름차순으로
*/
pcb_t *creat_proc_entry(PMv1_object *obj, uint64_t entry, uint8_t parid)
{
    // id 로직

    uint64_t target_chunk;
    uint64_t leading_zeros;

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
        obj->proc_comocc |= (1 << (3 - obj->occ_num));
    }

    pid = 63 - pid;

    obj->PMv1_mem[pid].id = pid;      // 프로세스의 id를 할당된 pid로 변경
    obj->PMv1_mem[pid].b_id = pid;    // 죽을때 쓸 id를 저장
    obj->PMv1_mem[pid].p_id = parid;  // 부모 id를 수정함
    obj->PMv1_mem[pid].proc_info = 0; // 정보를 0으로 수정

    // 메모리 로직
    // 128KB를 할당 리턴 된 메모리 스택 주소를 받음
    obj->PMv1_mem[pid].mm_addr = mm_creat(&mm_stack, INITIAL_PROC_SIZE);

    // 할당 후 주소를 줌
    // 자신의 주소를 알아내고
    uint64_t real_addr = mm_find(&mm_stack, obj->PMv1_mem[pid].mm_addr, 0);
    uint64_t *reg_val = (uint64_t *)(real_addr + (INITIAL_PROC_SIZE << 10) - 256);
    obj->PMv1_mem[pid].reg = (INITIAL_PROC_SIZE << 10) - 256; // 레지스터 위치

    obj->PMv1_mem[pid].pc = entry;
    obj->PMv1_mem[pid].sp = (uint64_t)reg_val;

    for (int i = 0; i < 31; i++)
        reg_val[i] = 0;              // x0~x30 초기화
    reg_val[31] = (uint64_t)reg_val; // SP 초기값
    reg_val[32] = entry;             // PC (ELR_EL1)
    reg_val[33] = 0x3C5;             // SPSR_EL1

    return &obj->PMv1_mem[pid];
}

pcb_t *creat_proc(PMv1_object *obj, void *task, uint8_t parid)
{
    return creat_proc_entry(obj, (uint64_t)task, parid);
}

/* 주소를 주면 변환해서 내주는 코드
    주소 -> 실제 주소
    pid
    type = 0 : low q로직
    type = 1 : high q로직
    cmd = 0 : 넣기
    cmd = 1 : 빼기
*/
uint8_t pm_low(PMv1_object *queue, uint8_t cmd, uint8_t val)
{
    if (cmd == 0)
    {
        queue->PMv1_lowqueue[queue->lowhead] = val;
        queue->lowhead = (queue->lowhead + 1) & 255;
        queue->lownum++;
        return 0;
    }

    if (queue->lownum == 0)
    {
        return 0;
    }
    uint8_t ret = queue->PMv1_lowqueue[queue->lowtail];
    queue->lowtail = (queue->lowtail + 1) & 255;
    queue->lownum--;
    return ret;
}

uint8_t pm_high(PMv1_object *queue, uint8_t cmd, uint8_t val)
{
    if (cmd == 0)
    {
        queue->PMv1_highqueue[queue->highhead] = val;
        queue->highhead = (queue->highhead + 1) & 255;
        queue->highnum++;
        return 0;
    }

    if (queue->highnum == 0)
    {
        return 0;
    }
    uint8_t ret = queue->PMv1_highqueue[queue->hightail];
    queue->hightail = (queue->hightail + 1) & 255;
    queue->highnum--;
    return ret;
}

// ! 정확히 작동하는지 확인 필요
uint8_t pm_qaddr(PMv1_object *queue, uint8_t type, uint8_t cmd, uint8_t val)
{
    if (type == 0)
    {
        return pm_low(queue, cmd, val);
    }

    return pm_high(queue, cmd, val);
}

// 프로세스 실행 함수
// 큐에 들어가 있는 대로 진행함
pcb_t *pm_run(PMv1_object *obj)
{
    uint8_t data;

    if (obj->highnum != 0)
    {
        data = pm_high(obj, 1, 0);

        puts("\nNext Proc ID: ");
        put_hex(data);

        // 안전 체크
        if (data >= PMV1_MAX_PROC)
            return &obj->PMv1_mem[0];

        if (data == 0)
            return &obj->PMv1_mem[0];

        if (data == PROC_SIGNAL)
        {
            // ! signal 처리
            return &obj->PMv1_mem[0];
        }

        return &obj->PMv1_mem[data];
    }

    else if (obj->lownum != 0)
    {
        data = pm_low(obj, 1, 0);

        // 다음 프로세스 확인
        puts("\nNext Proc ID: ");
        put_hex(data);
        puts("\nNext Proc ADDR: ");
        put_hex((uint64_t)current_proc);

        if (data >= PMV1_MAX_PROC)
            return &obj->PMv1_mem[0];

        if (data == 0)
            return &obj->PMv1_mem[0];

        return &obj->PMv1_mem[data];
    }
    return &obj->PMv1_mem[0];
}

/*
    proc to proc 전송 함수
    cmd = 0 : 메시지 전송 함수
    cmd = 1 : 메시지 수신 함수
    who가 towho에게 msg를 실행
*/
void ptp(PMv1_object *obj, uint8_t who, uint8_t towho, int8_t msg[64])
{
    pcb_t rece = obj->PMv1_mem[towho];
    if (rece.is_msgbox == FALSE)
    {
        // 메시지 넣는 로직
        rece.is_msgbox = TRUE;
        rece.from = who;
        for (int i = 0; i < 64; i++)
        {
            rece.msgbox[i] = msg[i];
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

/*
    프로세스 활성, 비활성 함수
    uint8_t cmd = 0 , uint8_t task의 주소 포인터 활성 상태로 전환
    uint8_t cmd = 1 , uintt_t task의 주소 포인터 비활성 상태로 전환
    uint8_t cmd = 2 , uint8_t task의 주소 및 메모리 해제
    요악 하면 cmd = 1은 wait, cmd = 2 는 kill
*/
void pm_awake(PMv1_object *obj, uint8_t cmd, pcb_t *proc)
{
    // pm_run의 대기 큐에 삽입
    if (cmd == 0)
    {
        pm_low(&pm_object, 0, proc->id);
    }
    // pm_run에서 삭제
    else
    {
        puts("make zombie");
        // 일단 비트를 수정하기
        proc->proc_info = cmd;  // cmd 값에 따라 달라짐
        proc->b_id = proc->id;  // 전 id를 id로 변경
        proc->id = PROC_SIGNAL; // 시그널값으로 변경

        uint8_t *ptr = (uint8_t *)proc;
        /*
        puts("\n Raw PCB memory: ");
        for (int i = 0; i < 16; i++)
        {
            put_hex(ptr[i]); // PCB 앞부분 16바이트를 1바이트씩 다 찍어봐
        }
        */
        // ! 프로세스가 차지한 공간을 가비지 컬랙터에게 줌

        if (cmd == 1)
        {
            pm_low(&pm_object, 0, proc->id);
        }
        else if (cmd == 2)
        {
            //
        }
    }
}
