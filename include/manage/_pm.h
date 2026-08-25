#ifndef __KERNEL_PM_H__
#define __KERNEL_PM_H__

#include "manage/_dm.h"
#include "tools/_dstruc.h"

typedef struct proc_regs_t
{
    uint64_t reg_x[31]; // x0 ~ x30
    uint64_t sp;        // 스택 포인터
    uint64_t spsr;      // SPSR_EL1
    uint64_t elr_el1;   // ELR_EL1 프로그램 카운터
} proc_regs_t;

typedef struct proc_msg_t
{
    uint8_t from;      // 누구에게 왔는지
    uint8_t is_call;   // 자신에게 읽으라고 했는지
    uint8_t is_msgbox; // 메시지 박스가 차있는지
    char *msgbox;      // 메세지
} proc_msg_t;

// fd 유니온 만들기
typedef struct ctrl_t
{
    uint32_t is_ctrl_alloc;
    uint8_t is_file; // 파일이 열려 있는지

    uint32_t file_offset;
    union
    {
        struct dcb_t *use_dev;  // 사용하는 디바이스
        struct fcb_t *use_file; // 사용하는 파일
    };

} ctrl_t;

typedef struct pcb_t
{
    struct proc_regs_t regs;

    // 정보 관련
    uint8_t id;       // 프로세스 id
    uint8_t p_id;     // 부모의 id
    uint16_t mm_addr; // 메모리 주소
    uint8_t state;    // 프로세스 상태(00 : 활성화, 01 : 휴면 상태, 10 : 정지 상태, 11 : 좀비 상태)

    // 메시지 관련
    struct proc_msg_t msgs;

    // 장치 관련
    struct ctrl_t control[MAX_CONTROL_NUM];

} __attribute__((aligned(8))) pcb_t;

typedef struct proto_t
{
    uint8_t rule;
    uint8_t *addr;

} proto_t;

// ! 전체적으로 개편이 필요함
typedef struct PMv1_object
{
    uint64_t *base; // 바닥 주소
    // 총 공간이 24KB 정도

    // 할당여부를 담당
    uint8_t is_alloc[MAX_PCB_SIZE];

    // 큐
    struct queue lowqueue;
    uint8_t lowbuf[MAX_PCB_SIZE];
    struct queue highqueue;
    uint8_t highbuf[MAX_PCB_SIZE];

    // 동적 배열로 바꾸기
    struct pcb_t PMv1_mem[MAX_PCB_SIZE]; // 최대 프로세스 수 만큼 만들기 8KB 정도 pcb의 배열

    // 프로토콜 관련
    struct proto_t proto_arr[MAX_PCB_SIZE];

} PMv1_object;

// 함수 선언

// init 만들기
void pm_init();
pcb_t *creat_proc(PMv1_object *obj, void *task, uint8_t parid);
pcb_t *pm_creat(PMv1_object *obj, uint64_t entry, uint8_t parid);
uint8_t pm_low(PMv1_object *queue, uint8_t cmd, uint8_t val);
uint8_t pm_high(PMv1_object *queue, uint8_t cmd, uint8_t val);
uint8_t pm_qaddr(PMv1_object *queue, uint8_t type, uint8_t cmd, uint8_t val);
pcb_t *pm_run(PMv1_object *obj);
void pm_awake(PMv1_object *obj, uint8_t cmd, pcb_t *proc);
void ptp(PMv1_object *obj, uint8_t who, uint8_t towho, int8_t msg[64]);

// 전역 구조체 선언
#define pm_object (*(PMv1_object *)PM_ADDR_START)

#endif
