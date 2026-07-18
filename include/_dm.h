#include "_types.h"
#include "_sect.h"
#include "_defs.h"

#ifndef __DM_H__
#define __DM_H__

// 드라이버 구조체
typedef struct device
{
    char *name;                       // 드라이버 이름
    uint64_t base_addr;               // 장치의 주소
    uint32_t irq_nr;                  // 본인의 IRQ 번호 저장
    int (*init)(void);                // 초기화 함수 포인터
    int (*read)(void *buf);           // 데이터 읽기 함수 포인터
    int (*write)(void *buf);          // 데이터 쓰기 함수 포인터
    void (*handler)(uint32_t irq_nr); // 인터럽트 발생 시 처리 로직
} device;

typedef struct DMv1_driver
{
    // 관리자 주소
    uint64_t *base;

    struct device DMv1_mem[MAX_DEVI_NUM];
} DMv1_driver;

static inline uint32_t check_glc(void)
{
    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    uint32_t irq_nr = iar & 0x3FF;
    return irq_nr;
}

static inline void reset_timer(void)
{
    uint32_t iar = *(volatile uint32_t *)(GIC_CPU_BASE + 0x0C);
    *(volatile uint32_t *)(GIC_CPU_BASE + 0x10) = iar;

    // 타이머 재설정
    asm volatile("msr cntp_tval_el0, %0" : : "r"(0x1000000));
}

// 전역 구조체 선언
// #define dm_driver (*(DMv1_driver *)DM_ADDR_START)

device *dm_creat(
    DMv1_driver *driv,
    uint32_t irq_nr,
    char *dev_name,
    uint64_t dev_addr,
    int (*init_func)(void),
    void (*handler_func)(uint32_t));

#pragma region not_imp

/*
int8_t dm_init(void);
int8_t dm_register_device(Driver *device);
int8_t dm_unregister_device(char *device_name);
int8_t dm_open(char *device_name, uint8_t mode);
int8_t dm_close(char *device_name);
int32_t dm_read(char *device_name, void *buf, uint32_t size);
int32_t dm_write(char *device_name, void *buf, uint32_t size);
int32_t dm_control(char *device_name, uint32_t cmd, void *arg);
void dm_interrupt_handler(uint32_t device_id);
int8_t dm_get_device_info(char *device_name, void *info);
uint8_t dm_is_device_ready(char *device_name);
int8_t dm_set_device_config(char *device_name, void *config);
int8_t dm_get_device_status(char *device_name, void *status);
void dm_sync(void);
*/

#pragma endregion

#endif
