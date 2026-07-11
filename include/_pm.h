#include "_defs.h"
#include "_types.h"

#ifndef __PM_H__
#define __PM_H__

// pcb 104B
// 나중에 프로세스가 생성되고 레지스터 공간 따로 할당
typedef struct pcb_t
{
    uint64_t reg_x[31]; // x0 ~ x30
    uint64_t sp;        // 스택 포인터
    uint64_t spsr;      // SPSR_EL1
    uint64_t elr_el1;   // ELR_EL1 프로그램 카운터

    uint8_t id;         // 프로세스 id
    uint8_t p_id;       // 부모의 id
    uint8_t b_id;       // 죽을때 쓸 id
    uint16_t proc_info; // 프로세스 정보
    uint16_t mm_addr;   // 메모리 주소

    uint8_t is_msgbox : 1; // 메시지 박스가 차있는지
    uint8_t is_call : 1;   // 자신에게 읽으라고 했는지
    uint8_t state : 2;     // 프로세스 상태(00 : 활성화, 01 : 휴면 상태, 10 : 정지 상태, 11 : 좀비 상태)
    uint8_t padding : 4;   // 패딩값
    uint8_t from;          // 누구에게 왔는지
    uint8_t msgbox[64];    // 메세지

} __attribute__((aligned(8))) pcb_t;

typedef struct PMv1_object
{
    uint64_t temp_x[31];   // 임시 레지스터
    uint64_t temp_exce[8]; // irq에 사용하는 임시 레지스터

    uint64_t *base; // 바닥 주소
    // 총 공간이 24KB 정도
    uint8_t proc_comocc : 4; // 그중에서 어떤 proc_occ 이 사용되지 않았는지
    uint8_t proc_comscj : 4; // 프로세서 pm_rum에 들어가는 관리하는
    uint8_t lownum;          // low에 들어있는 프로세스 수
    uint8_t highnum;         // high에 들어있는 프로세스 수
    uint8_t lowhead;         // 원형큐 머리
    uint8_t lowtail;         // 원형큐 꼬리
    uint8_t highhead;        // 원형큐 머리
    uint8_t hightail;        // 원형큐 꼬리

    pcb_t PMv1_mem[MAX_PCB_SIZE];         // 최대 프로세스 수 만큼 만들기 8KB 정도 pcb의 배열
    uint8_t PMv1_lowqueue[MAX_PCB_SIZE];  // 프로세스 low q
    uint8_t PMv1_highqueue[MAX_PCB_SIZE]; // 프로세스 high q

    uint64_t proc_occ[MAX_PCB_BITSIZE];    // 어떤 프로세스 pid 를 사용하지 않았는지
    uint64_t occ_num;                      // occ 숫자 넣기 레지스터에 넣기 좋도록 64bit를 씀
    uint64_t proc_scj[MAX_PCB_BITSIZE];    // 스케줄러에 들어갈 task들의 우선순위를 계산하기 위한 배열 순환돌때 여기다가 적는다
    uint64_t proc_priscj[MAX_PCB_BITSIZE]; // 스캐줄러에 들어갈 task들의 우선순위가 적힌 배열
} PMv1_object;

// 함수 선언

// init 만들기

pcb_t *creat_proc(PMv1_object *obj, void *task, uint8_t parid);
pcb_t *creat_proc_entry(PMv1_object *obj, uint64_t entry, uint8_t parid);
uint8_t pm_low(PMv1_object *queue, uint8_t cmd, uint8_t val);
uint8_t pm_high(PMv1_object *queue, uint8_t cmd, uint8_t val);
uint8_t pm_qaddr(PMv1_object *queue, uint8_t type, uint8_t cmd, uint8_t val);
pcb_t *pm_run(PMv1_object *obj);
void pm_awake(PMv1_object *obj, uint8_t cmd, pcb_t *proc);
void ptp(PMv1_object *obj, uint8_t who, uint8_t towho, int8_t msg[64]);

// 전역 구조체 선언
#define pm_object (*(PMv1_object *)PM_ADDR_START)

#endif
