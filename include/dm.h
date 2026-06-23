#include "../include/types.h"

#ifndef __DM_H__
#define __DM_H__

// 드라이버 구조체
typedef struct Driver
{
    char *name;              // 드라이버 이름
    int (*init)(void);       // 초기화 함수 포인터
    int (*read)(void *buf);  // 데이터 읽기 함수 포인터
    int (*write)(void *buf); // 데이터 쓰기 함수 포인터
    void (*handler)(void);   // 인터럽트 발생 시 처리 로직
} Driver;

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
