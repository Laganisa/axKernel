#include "_dm.h"

// 장치 등록 함수
device *dm_creat(DMv1_driver *driv, uint32_t irq_nr)
{
    if (irq_nr > MAX_DEVI_NUM)
    {
        return 0;
    }

    device *new_device = &(driv->DMv1_mem[irq_nr]);

    // 삽입부
    new_device->irq_nr = irq_nr;
    new_device->name = 1;
    return new_device;
}