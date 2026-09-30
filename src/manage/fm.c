#include "manage/_fm.h"
#include "global/_io.h"
#include "manage/_mm.h"
#include "tools/_asm.h"
#include "global/_debug.h"
#include "global/_alloc.h"
#include "tools/_hash.h"

void fm_init(uint64_t *addr)
{
    fm_record->base = addr;
    fm_record->cur_ptr = 0;
    fm_record->all_file_num = 1;
    fm_record->root = create_node(1);
    fm_record->dir_root = create_node(1);
}

/*
    파일의 이름, 크기, 권한을 받아서 파일 타입 구조체를 리턴함
*/
fcb_t *fm_create(FMv3_record *reco, char *name, uint32_t size, uint16_t auth)
{
    // 파일 검사
    if (size > MAX_FILE_SIZE)
        return 0;

    if (reco->all_file_num >= MAX_FILE_NUM)
        return NULL;

    fcb_t *new_file = &(reco->FMv3_mem[reco->all_file_num]);

    uint16_t value = reco->all_file_num;

    // 파일 삽입
    int ret = bpt_insert(reco->root, name, value);

    if (ret == 0)
    {
        return NULL;
    }

    if (new_file == NULL)
    {
        return 0;
    }

    for (int i = 0; i < MAX_FILE_NAME; i++)
    {
        new_file->alias[i] = name[i];
    }

    new_file->is_alloc = 1;
    new_file->lens = size >> 10;
    new_file->fid = (uint16_t)reco->all_file_num;
    new_file->auth = auth;

    reco->all_file_num += 1;

    return new_file;
}

uint8_t fm_delete(FMv3_record *reco, char *name)
{
    // 권한 확인
    // TODO: 권한 확인 로직 추가 필요
    /*
    uint16_t *file_index = bpt_search(reco->root, name);

    if (file_index == NULL)
    {
        return NULL;
    }
    // 현재 파일의 권한이 삭제가 가능한지
    reco->FMv3_mem[*file_index].auth = 0;
    */

    return bpt_delete(reco->root, name);
}

// 디렉토리 생성
fcb_e *fm_dir_create(
    FMv3_record *reco,
    char *name,
    uint32_t p_fid,
    uint16_t depth,
    uint16_t auth)
{

    if (reco->all_dir_num >= MAX_FILE_NUM)
        return NULL;

    fcb_e *new_dir = &(reco->FMv3_dir_mem[reco->all_dir_num]);

    uint16_t value = reco->all_file_num;

    // 파일 삽입
    int ret = bpt_insert(reco->root, name, value);

    if (ret == 0)
    {
        return NULL;
    }

    if (new_dir == NULL)
    {
        return 0;
    }

    // 이름 복사
    for (int i = 0; i < MAX_FILE_NAME; i++)
    {
        new_dir->alias[i] = name[i];
    }

    new_dir->alias[MAX_FILE_NAME] = '\0';
    new_dir->p_fid = p_fid;
    new_dir->depth = depth;
    new_dir->fid = (uint16_t)reco->all_dir_num;
    new_dir->auth = auth;

    reco->all_dir_num += 1;

    return new_dir;
}

// 디렉토리 삭제
uint8_t fm_dir_delete(FMv3_record *reco, char *name)
{
}

void fm_execute(FMv3_record *reco)
{
    if (reco == NULL)
    {
        return NULL; // 파일 관리자 미초기화
    }

    if (reco->base == NULL)
    {
        return NULL; // 베이스 주소 미설정
    }

    // 파일 개수와 마지막 주소 검증
    if (reco->all_file_num > MAX_FILE_NUM)
    {
        reco->all_file_num = 0; // 비정상적인 파일 개수 초기화
    }

    if (reco->cur_ptr > MAX_FILE_NUM)
    {
        reco->cur_ptr = 0; // 현재 포인터 초기화
    }

    // ! 시스템 동기화 (향후 구현)
    // ! 구현 까지 시간이 좀 걸릴 듯
    // ! fm_sync(reco);
}

uint32_t fm_write(FMv3_record *reco, fcb_t *file, void *buf, uint32_t size, uint32_t offset)
{
    if (file == 0)
    {
        return 0;
    }

    uint32_t file_size = (uint32_t)file->lens << 10;
    if (offset >= file_size)
    {
        return 0;
    }

    if (size > (file_size - offset))
    {
        size = file_size - offset;
    }

    uint8_t *dst_addr = (uint8_t *)fm_data_addr(reco, file) + offset;
    memcpy(dst_addr, (uint8_t *)buf, size);
    return size;
}

uint32_t fm_read(FMv3_record *reco, fcb_t *file, void *buf, uint32_t size, uint32_t offset)
{
    if (file == NULL)
    {
        return 0;
    }

    uint32_t file_size = (uint32_t)file->lens << 10;
    if (offset >= file_size)
    {
        return 0;
    }
    if (size > (file_size - offset))
    {
        size = file_size - offset;
    }
    uint8_t *src_addr = (uint8_t *)fm_data_addr(reco, file) + offset;
    memcpy((uint8_t *)buf, src_addr, size);

    return size;
}

// 파일 목록 조회
// 주어진 경로의 디렉토리 내 파일 목록을 출력
void fm_list(FMv3_record *reco, int8_t *path)
{
}

// 파일 찾기
fcb_t *fm_find(FMv3_record *reco, char *name)
{
    uint16_t ret = bpt_search(reco->root, name);

    if (ret != 0)
    {
        return &(reco->FMv3_mem[ret]);
    }
    return 0;
}

void *fm_data_addr(FMv3_record *reco, fcb_t *file)
{
    return (void *)(reco->base + (file->fid * MAX_FILE_SIZE));
}
