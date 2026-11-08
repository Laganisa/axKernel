#include "manage/_fm.h"
#include "global/_io.h"
#include "manage/_mm.h"
#include "tools/_asm.h"
#include "tools/_fs.h"
#include "global/_debug.h"
#include "global/_alloc.h"
#include "tools/_hash.h"
#include "tools/_dsk_fs.h"

void fm_init(uint64_t *addr)
{
    fm_record->base = addr;
    fm_record->cur_ptr = 0;
    fm_record->all_file_num = 1;
    fm_record->all_dir_num = 1;
    fm_record->root = create_node(1);
    fm_record->dir_root = create_node(1);
    fm_dir_create(fm_record, "root", 0, 0, 777);
}

/*
    파일의 이름, 크기, 권한을 받아서 파일 타입 구조체를 리턴함
*/
fcb_t *fm_create(
    FMv3_record *reco,
    char *name,
    uint32_t size,
    uint16_t auth)
{
    uint32_t name_length = 0;
    if (name != NULL)
    {
        while (name_length <= MAX_FILE_NAME && name[name_length] != '\0')
        {
            name_length++;
        }
    }

    if (reco == NULL || reco->root == NULL || name == NULL ||
        name_length == 0 || name_length > MAX_FILE_NAME ||
        size > MAX_FILE_SIZE || auth > 0x3FF ||
        reco->all_file_num >= MAX_FILE_NUM)
    {
        return NULL;
    }

    fcb_e *dir = fm_dir_find(reco, "root");
    if (dir == NULL || dir->dir_num >= MAX_DIR_FILE_NUM)
    {
        return NULL;
    }

    uint32_t first_sector = AXV_EMPTY;
    if (fm_dsk_ready() && size != 0)
    {
        first_sector = axv_create(size);
        if (first_sector == AXV_EMPTY)
        {
            return NULL;
        }
    }

    uint16_t file_id = reco->all_file_num;
    if (bpt_insert(reco->root, name, file_id) == 0)
    {
        if (first_sector != AXV_EMPTY)
        {
            axv_delete(first_sector, size);
        }
        return NULL;
    }

    fcb_t *new_file = &reco->FMv3_mem[file_id];
    memset(new_file, 0, sizeof(*new_file));
    for (uint32_t i = 0; i < MAX_FILE_NAME && name[i] != '\0'; i++)
    {
        new_file->alias[i] = name[i];
    }

    new_file->is_alloc = TRUE;
    new_file->lens = size;
    new_file->fid = file_id;
    new_file->auth = auth;
    new_file->dsk_id = first_sector;

    reco->all_file_num++;
    if (!fm_dir_in(reco, dir, file_id))
    {
        reco->all_file_num--;
        bpt_delete(reco->root, name);
        new_file->is_alloc = FALSE;
        if (first_sector != AXV_EMPTY)
        {
            axv_delete(first_sector, size);
        }
        return NULL;
    }

    if (!fm_dsk_ready() && size != 0)
    {
        memset(fm_data_addr(reco, new_file), 0, size);
    }
    return new_file;
}

uint8_t fm_delete(FMv3_record *reco, char *name)
{
    if (reco == NULL || reco->root == NULL || name == NULL)
    {
        return FALSE;
    }

    uint16_t *file_id_ptr = bpt_search(reco->root, name);
    if (file_id_ptr == NULL || *file_id_ptr == 0 ||
        *file_id_ptr >= reco->all_file_num)
    {
        return FALSE;
    }

    uint16_t file_id = *file_id_ptr;
    fcb_t *file = &reco->FMv3_mem[file_id];
    if (!file->is_alloc || file->fid != file_id)
    {
        return FALSE;
    }
    fcb_e *parent = NULL;
    for (uint16_t i = 1; i < reco->all_dir_num; i++)
    {
        fcb_e *directory = &reco->FMv3_dir_mem[i];
        if (directory->dir_num > MAX_DIR_FILE_NUM)
        {
            return FALSE;
        }
        for (uint16_t j = 0; j < directory->dir_num; j++)
        {
            if (directory->files[j] == file_id)
            {
                parent = directory;
                break;
            }
        }
        if (parent != NULL)
        {
            break;
        }
    }
    if (parent == NULL)
    {
        return FALSE;
    }

    uint8_t queued = FALSE;
    if (fm_dsk_ready() && file->lens != 0)
    {
        if (!fm_dsk_retire(file->dsk_id, file->lens))
        {
            return FALSE;
        }
        queued = TRUE;
    }

    if (!bpt_delete(reco->root, name))
    {
        if (queued)
        {
            fm_dsk_cancel_retire(file->dsk_id, file->lens);
        }
        return FALSE;
    }

    if (!fm_dir_out(reco, parent, file_id))
    {
        return FALSE;
    }
    file->is_alloc = FALSE;
    return TRUE;
}

// 디렉토리 생성
fcb_e *fm_dir_create(
    FMv3_record *reco,
    char *name,
    uint32_t p_fid,
    uint16_t depth,
    uint16_t auth)
{
    if (reco == NULL || reco->dir_root == NULL ||
        name == NULL || name[0] == '\0' ||
        reco->all_dir_num >= MAX_DIR_NUM ||
        depth > 2 || auth > 0x3FF)
    {
        return NULL;
    }

    fcb_e *parent = NULL;
    if (depth == 0)
    {
        if (p_fid != 0)
        {
            return NULL;
        }
    }
    else
    {
        if (p_fid == 0 || p_fid >= reco->all_dir_num ||
            p_fid >= MAX_DIR_NUM)
        {
            return NULL;
        }
        parent = &reco->FMv3_dir_mem[p_fid];
        if (parent->depth + 1 != depth ||
            parent->dir_dnum >= MAX_DIR_FILE_NUM)
        {
            return NULL;
        }
    }

    fcb_e *new_dir =
        &(reco->FMv3_dir_mem[reco->all_dir_num]);

    uint16_t value = reco->all_dir_num;

    // 디렉토리 삽입
    int ret = bpt_insert(
        reco->dir_root, name, value);

    if (ret == 0)
    {
        return NULL;
    }

    memset(new_dir, 0, sizeof(*new_dir));
    // 이름 복사
    for (int i = 0; i < MAX_FILE_NAME; i++)
    {
        new_dir->alias[i] = name[i];
    }

    new_dir->alias[MAX_FILE_NAME] = '\0';

    for (int i = 0; i < MAX_DIR_FILE_NUM; i++)
    {
        // 루트 파일을 가리키도록 초기화
        new_dir->files[i] = 0;
    }

    for (int i = 0; i < MAX_DIR_FILE_NUM; i++)
    {
        // 루트 폴더을 가리키도록 초기화
        new_dir->dirs[i] = 0;
    }

    if (depth == 0)
    {
        new_dir->p_fid = 0; // 루트 디렉토리
    }
    else
    {
        new_dir->p_fid = p_fid;
        parent->dirs[parent->dir_dnum] = (uint16_t)reco->all_dir_num;
        parent->dir_dnum++;
    }

    new_dir->depth = depth;

    new_dir->fid = (uint16_t)reco->all_dir_num;
    new_dir->auth = auth;
    new_dir->dir_num = 0;
    new_dir->dir_dnum = 0;

    reco->all_dir_num += 1;

    return new_dir;
}

// 디렉토리 삭제
uint8_t fm_dir_delete(FMv3_record *reco, char *name)
{
    // TODO: 권한 확인 로직 추가 필요
    return bpt_delete(reco->dir_root, name);
}

void fm_execute(FMv3_record *reco)
{
    if (reco == NULL)
    {
        return; // 파일 관리자 미초기화
    }

    if (reco->base == NULL)
    {
        return; // 베이스 주소 미설정
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

static uint8_t fm_dsk_copy(
    uint32_t source,
    uint32_t source_length,
    uint32_t destination,
    uint32_t destination_length)
{
    uint8_t buffer[AXV_SECTOR_SIZE] __attribute__((aligned(16)));
    uint32_t offset = 0;
    while (offset < source_length)
    {
        uint32_t chunk = source_length - offset;
        if (chunk > sizeof(buffer))
        {
            chunk = sizeof(buffer);
        }
        if (axv_read(source, source_length, offset, buffer, chunk) != chunk ||
            axv_write(destination, destination_length, offset, buffer, chunk) != chunk)
        {
            return FALSE;
        }
        offset += chunk;
    }
    return TRUE;
}

uint32_t fm_write(
    FMv3_record *reco,
    fcb_t *file,
    void *buf,
    uint32_t size,
    uint32_t offset)
{
    if (reco == NULL || file == NULL || !file->is_alloc ||
        (buf == NULL && size != 0) || size == 0 ||
        size > (uint32_t)-1 - offset)
    {
        return 0;
    }

    uint32_t file_size = file->lens;
    uint32_t end = offset + size;
    if (end > MAX_FILE_SIZE)
    {
        return 0;
    }

    if (!fm_dsk_ready() && file->dsk_id != AXV_EMPTY)
    {
        return 0;
    }

    if (fm_dsk_ready())
    {
        if (end > file_size)
        {
            if (file_size != 0 && file->dsk_id == AXV_EMPTY)
            {
                return 0;
            }

            uint32_t new_sector = axv_create(end);
            if (new_sector == AXV_EMPTY)
            {
                return 0;
            }
            if (file_size != 0 &&
                (!axv_check_chain(file->dsk_id, file_size) ||
                 !fm_dsk_copy(file->dsk_id, file_size, new_sector, end)))
            {
                axv_delete(new_sector, end);
                return 0;
            }
            if (axv_write(new_sector, end, offset, buf, size) != size)
            {
                axv_delete(new_sector, end);
                return 0;
            }
            if (file_size != 0 &&
                !fm_dsk_retire(file->dsk_id, file_size))
            {
                axv_delete(new_sector, end);
                return 0;
            }

            file->dsk_id = new_sector;
            file->lens = end;
            return size;
        }

        if (file->lens == 0 || file->dsk_id == AXV_EMPTY)
        {
            return 0;
        }
        return axv_write(
            file->dsk_id,
            file_size,
            offset,
            buf,
            size);
    }

    if (offset > file_size)
    {
        memset((uint8_t *)fm_data_addr(reco, file) + file_size, 0, offset - file_size);
    }
    uint8_t *dst_addr = (uint8_t *)fm_data_addr(reco, file) + offset;
    memcpy(dst_addr, (uint8_t *)buf, size);
    if (end > file_size)
    {
        file->lens = end;
    }
    return size;
}

uint32_t fm_read(
    FMv3_record *reco,
    fcb_t *file,
    void *buf,
    uint32_t size,
    uint32_t offset)
{
    if (reco == NULL || file == NULL || !file->is_alloc ||
        (buf == NULL && size != 0))
    {
        return 0;
    }

    uint32_t file_size = file->lens;
    if (offset >= file_size)
    {
        return 0;
    }
    if (size > (file_size - offset))
    {
        size = file_size - offset;
    }
    if (size == 0)
    {
        return 0;
    }
    if (!fm_dsk_ready() && file->dsk_id != AXV_EMPTY)
    {
        return 0;
    }
    if (fm_dsk_ready())
    {
        if (file->dsk_id == AXV_EMPTY)
        {
            return 0;
        }
        return axv_read(
            file->dsk_id,
            file_size,
            offset,
            buf,
            size);
    }

    uint8_t *src_addr = (uint8_t *)fm_data_addr(reco, file) + offset;
    memcpy((uint8_t *)buf, src_addr, size);

    return size;
}

// 성공 여부를 반환함
// 디렉토리에 파일을 추가하거나 제거하는 함수
uint8_t fm_dir_in(
    FMv3_record *reco,
    fcb_e *dir,
    uint16_t file_id)
{
    // TODO: 권한 확인 로직 추가 필요

    // 디렉토리 검사
    if (reco == NULL || dir == NULL ||
        dir->dir_num >= MAX_DIR_FILE_NUM)
    {
        return 0;
    }

    // file_id 검사
    if (file_id == 0 || file_id >= reco->all_file_num)
    {
        return 0; // 파일 ID가 유효하지 않음
    }

    dir->files[dir->dir_num] = file_id;
    dir->dir_num++;
    return 1;
}

uint8_t fm_dir_out(
    FMv3_record *reco,
    fcb_e *dir,
    uint16_t file_id)
{
    if (reco == NULL || dir == NULL || dir->dir_num > MAX_DIR_FILE_NUM ||
        file_id == 0 || file_id >= reco->all_file_num)
    {
        return FALSE;
    }

    for (uint16_t i = 0; i < dir->dir_num; i++)
    {
        if (dir->files[i] != file_id)
        {
            continue;
        }
        for (uint16_t j = i + 1; j < dir->dir_num; j++)
        {
            dir->files[j - 1] = dir->files[j];
        }
        dir->files[--dir->dir_num] = 0;
        return TRUE;
    }

    return FALSE;
}

// 파일 목록 조회
// 주어진 경로의 디렉토리 내 파일 목록을 출력
// TODO: 나중에 하기!
void fm_list(FMv3_record *reco, int8_t *path)
{
}

// 파일 찾기
fcb_t *fm_find(FMv3_record *reco, char *name)
{
    uint16_t *ret = bpt_search(reco->root, name);

    if (ret != NULL)
    {

        return &(reco->FMv3_mem[*ret]);
    }

    return NULL;
}

fcb_e *fm_dir_find(FMv3_record *reco, char *name)
{
    uint16_t *ret = bpt_search(reco->dir_root, name);

    if (ret != NULL)
    {
        return &(reco->FMv3_dir_mem[*ret]);
    }

    return NULL;
}

fm_exec_hdr_t *fm_data_addr(FMv3_record *reco, fcb_t *file)
{
    fm_exec_hdr_t *new_hdr;
    new_hdr = (fm_exec_hdr_t *)((uint8_t *)reco->base + ((uint32_t)file->fid * MAX_FILE_SIZE));
    return new_hdr;
}
