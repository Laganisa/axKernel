#include "_types.h"
#include "_sect.h"
#include "_defs.h"
#include "_macro.h"

#ifndef __DM_H__
#define __DM_H__

// 드라이버 구조체
typedef struct dcb_t
{
    char *name;         // 드라이버 이름
    uint64_t base_addr; // 장치의 주소
    uint32_t irq_nr;    // 본인의 IRQ 번호 저장

    int (*init)(void); // 초기화 함수 포인터
    int (*open)(void);
    int (*close)(void);
    int (*read)(void *buf);           // 데이터 읽기 함수 포인터
    int (*write)(void *buf);          // 데이터 쓰기 함수 포인터
    void (*handler)(uint32_t irq_nr); // 인터럽트 발생 시 처리 로직
} dcb_t;

typedef struct DMv1_driver
{
    // 관리자 주소
    uint64_t *base;

    struct dcb_t DMv1_mem[MAX_DEVI_NUM];
} DMv1_driver;

// ! 이거 왜 있지?
static inline uint32_t check_glc(void)
{
    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    uint32_t irq_nr = iar & 0x3FF;
    return irq_nr;
}

// ! 이것도 왜 있지?
static inline void reset_timer(void)
{
    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    *(volatile uint32_t *)(GIC_CPU_BASE + 0x10) = iar;

    // 타이머 재설정
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));
}

// 전역 구조체 선언
// 나중에 메모리 맵이 확정되면 추가
#define dm_driver ((DMv1_driver *)DM_ADDR_START)

dcb_t *dm_creat(DMv1_driver *driv, uint32_t irq_nr, dcb_t dev);
dcb_t *dm_find(DMv1_driver *devi, const char *name);

int uart_dev_write(void *buf);
int uart_dev_read(void *buf);
void uart_dev_init();

#endif
