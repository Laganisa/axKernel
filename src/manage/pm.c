// #include "types.h"
// #include "defs.h"
#include "io.h"
#include "sync.h"

#include "mm.h"
// #include "pm.h"

#include "debug.h"

/*
    주소를 주면 변환해서 내주는 코드

    주소 -> 실제 주소
    pid
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
    uint8_t sec = 0; // 선택한 것

    if (obj->highnum != 0)
    {
        log("case A");
        data = pm_high(obj, 1, 0);

        // 안전 체크
        if (data >= PMV1_MAX_PROC || data == 0)
            return &obj->PMv1_mem[INIT_PROC_SECT];

        // 프로세스 좀비
        if (data == PROC_ZOMB)
        {
            // ! 좀비 처리
            // ! 그냥 init에 붙여두고 정리하면 될듯
            return &obj->PMv1_mem[INIT_PROC_SECT];
        }

        // 프로세스 대기
        if (data == PROC_DORM)
        {
        }

        return &obj->PMv1_mem[data];
    }

    else if (obj->lownum != 0)
    {

        data = pm_low(obj, 1, 0);

        if (data >= PMV1_MAX_PROC || data == 0)
            return &obj->PMv1_mem[INIT_PROC_SECT];

        return &obj->PMv1_mem[data];
    }

    return &obj->PMv1_mem[INIT_PROC_SECT];
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
        dump("id", proc->id);
        pm_low(&pm_object, 0, proc->id);
    }
    else
    {
        // 프로세스를 종료 상태로 표시
        proc->state = PROC_ZOMB;
        proc->b_id = proc->id; // 종료 시 원래 pid를 기록
        // id는 유지해서 식별을 보존

        uint8_t *ptr = (uint8_t *)proc;

        // ! 프로세스가 차지한 공간을 가비지 컬랙터에게 줌

        // 종료 상태인 프로세스는 스케줄러 대기 큐에 다시 넣지 않는다
        if (cmd == 2)
        {
            // 추가 삭제/해제 로직이 필요하면 여기에 구현
        }
    }
}
