#include "_dm.h"

// 장치 등록 함수
device *dm_creat(
    DMv1_driver *driv,
    uint32_t irq_nr,
    char *dev_name,
    uint64_t dev_addr,
    int (*init_func)(void),
    void (*handler_func)(uint32_t))
{
    if (irq_nr >= MAX_DEVI_NUM)
    {
        return NULL;
    }

    // 이미 등록되어 있는지 체크 (중복 등록 방지)
    if (driv->DMv1_mem[irq_nr].name != NULL)
    {
        return NULL;
    }

    // 공간을 주고
    device *new_device = &(driv->DMv1_mem[irq_nr]);

    // 값 할당
    new_device->irq_nr = irq_nr;
    new_device->name = dev_name;
    new_device->base_addr = dev_addr;
    new_device->init = init_func;
    new_device->handler = handler_func;

    // 초기화 함수가 제공되었다면 즉시 실행
    if (new_device->init != NULL)
    {
        if (new_device->init() != 0)
        {
            return NULL;
        }
    }

    return new_device;
}

void dm_init(void)
{
}
uint8_t timer_init(void)
{
    // 타이머 초기화
    asm volatile("msr cntp_tval_el0, %0" : : "r"(TIMER_TICK));
    return 0;
}
