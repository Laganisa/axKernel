#include "global/_io.h"
#include "handler/_sync.h"
#include "manage/_mm.h"
#include "global/_debug.h"

/*
    프로세스 간의 작업과 관련한 파일
*/

/*
    프로세스 실행 함수
    큐에 들어가 있는 대로 진행함
    큐에 있는 프로세스를 리턴함
*/

pcb_t *pm_run(PMv1_object *obj)
{
    uint8_t data; // pm 큐에서 뽑은 id 값

    if (!(obj->highqueue.empty(&(obj->highqueue))))
    {
        data = obj->highqueue.pop(&(obj->highqueue));
    }
    else if (!(obj->lowqueue.empty(&(obj->lowqueue))))
    {
        data = obj->lowqueue.pop(&(obj->lowqueue));
    }
    else
    {
        // 큐가 비어있으면 ROOT로 리턴
        return &obj->PMv1_mem[ROOT_PROC_SECT];
    }

    // 범위를 넘었다면 ROOT 프로세스를 리턴하기
    if (data >= PMV1_MAX_PROC || data == 0)
    {
        return &obj->PMv1_mem[ROOT_PROC_SECT];
    }

    return &obj->PMv1_mem[data];
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
        obj->lowqueue.push(&(obj->lowqueue), proc->id);
        // pm_low(&pm_object, 0, proc->id);
    }
    // 여기 하단은 호출되지 않음
    else
    {
        // ! 나중에 만들기

        // 휴면
        if (cmd == 1)
        {
            proc->state = PROC_DORM;
        }
        // 정지
        else if (cmd == 2)
        {
            proc->state = PROC_STOP;
        }
        // 좀비
        else if (cmd == 3)
        {
            proc->state = PROC_ZOMB;
        }

        /*
            id는 유지해서 식별을 보존
            종료 시 원래 pid를 기록
        */

        // ? 이거 왜 있음?
        uint8_t *ptr = (uint8_t *)proc;

        // ! 프로세스가 차지한 공간을 가비지 컬랙터에게 줌

        // 종료 상태인 프로세스는 스케줄러 대기 큐에 다시 넣지 않는다
        if (cmd == 2)
        {
            // 추가 삭제/해제 로직이 필요하면 여기에 구현
        }
    }
}

/*
    proc to proc 전송 함수
    cmd = 0 : 메시지 전송 함수
    cmd = 1 : 메시지 수신 함수
    who가 towho에게 msg를 실행
*/
void ptp(
    PMv1_object *obj,
    uint8_t who,
    uint8_t towho,
    uint8_t msg[64],
    uint8_t len)
{
    pcb_t *rece = &obj->PMv1_mem[towho];

    if (rece->msgs.is_msgbox == FALSE)
    {
        log("who");

        if (len > 64)
        {
            return;
        }

        // 메시지 넣는 로직
        rece->msgs.is_msgbox = TRUE;
        rece->msgs.from = who;
        rece->msgs.len = len;

        for (int i = 0; i < len; i++)
        {
            dump("msg", msg[i]);
            rece->msgs.msgbox[i] = msg[i];
        }

        // towho의 우선순위를 증가시켜 바로 입력 받을 수 있도록
        // ! 수정하기
        obj->lowqueue.pop(&(obj->lowqueue));           // 대상자를 빼고
        obj->highqueue.push(&(obj->highqueue), towho); // 대상자를 넣고
    }
    else
    {
        log("WHY");
        // ! 예외 처리
        // 메시지 박스가 차 있다면?
        // 그냥 안 보내면 되지 않나?
        // 아니면 그냥 for 문을 돌린다음에 처리하는것도 생각 중
    }
}