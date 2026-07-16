#include "_fm.h"
#include "_io.h"
#include "_mm.h"
#include "_asm.h"
#include "_debug.h"
#include "_alloc.h"
#include "_hash.h"

void *memset(void *ptr, int value, size_t num)
{
    unsigned char *p = (unsigned char *)ptr;

    for (size_t i = 0; i < num; i++)
    {
        p[i] = (unsigned char)value;
    }

    return ptr;
}

void fm_init(uint64_t *addr)
{
    fm_record->base = addr;
    fm_record->cur_ptr = 0;
    fm_record->root = create_node(1);
}

#pragma region fs

bpt_node *create_node(uint8_t leaf)
{
    bpt_node *new_node = heap_alloc(sizeof(bpt_node));

    if (new_node == NULL)
    {
        return NULL;
    }

    memset(new_node, 0, sizeof(bpt_node));

    new_node->leaf = leaf;
    new_node->data_num = 0;
    new_node->next = NULL;

    return new_node;
}

bpt_node *search_leaf(bpt_node *root, uint64_t key)
{
    bpt_node *pre = root;
    while (!pre->leaf)
    {
        int i;
        for (i = 0; i < pre->data_num; i++)
        {
            if (key < pre->fnv1a_hash_key[i])
                break;
        }

        pre = pre->child[i];
    }

    // 리프노드가 나오겠지?
    return pre;
}

void insert(bpt_node *root, char *name, uint16_t value)
{

    uint64_t key1 = fnv1a_hash_64(name);
    uint64_t key2 = djb2_hash_64(name);
    bpt_node *leaf = search_leaf(root, key1);

    if (leaf->data_num < MAX_BPT_NODE_NUM - 1)
    {
        // 그냥 삽입
        int pos = 0;

        while (pos < leaf->data_num && leaf->fnv1a_hash_key[pos] < key1)
        {
            pos++;
        }
        for (int i = leaf->data_num; i > pos; i--)
        {
            leaf->fnv1a_hash_key[i] = leaf->fnv1a_hash_key[i - 1];
            leaf->djb2_hash_Key[i] = leaf->djb2_hash_Key[i - 1];
            leaf->data[i] = leaf->data[i - 1];
        }
        leaf->fnv1a_hash_key[pos] = key1;
        leaf->djb2_hash_Key[pos] = key2;
        leaf->data[pos] = value;

        leaf->data_num++;
    }
    else
    {
        // Split
    }
}

uint16_t *search(bpt_node *root, char *name)
{
    uint64_t key = fnv1a_hash_64(name);

    bpt_node *leaf = search_leaf(root, key);

    if (leaf == NULL)
        return NULL;

    for (int i = 0; i < leaf->data_num; i++)
    {
        if (leaf->fnv1a_hash_key[i] == key)
        {
            return leaf->data[i];
        }
    }

    return NULL;
}

#pragma endregion

/*
    파일의 경로, 파일 크기, 파일 위치, 디랙토리 여부
*/
fcb_t *fm_create(FMv3_record *reco, int8_t *path, uint32_t size, uint8_t ok_dir)
{

    // 파일 검사
    if (size > MAX_FILE_SIZE)
        return 0;
    if (ok_dir && size > MAX_DIR_SIZE)
        return 0;

    fcb_t *new_file = NULL;

    // 남는 위치에 삽입
    new_file = &reco->FMv3_mem[reco->all_num];

    uint16_t value = reco->all_num;
    // 파일 삽입
    insert(reco->root, path, value);

    if (new_file == NULL)
    {
        return 0;
    }

    // 메타데이터 채우기 (기존 fm_create와 유사)
    for (int i = 0; i < MAX_FILE_NAME; i++)
    {
        new_file->alias[i] = path[i];
    }

    new_file->is_dir = ok_dir;
    new_file->is_alloc = 1;
    new_file->depth = (path[8] == 0x20) ? 0 : (path[17] == 0x20 ? 1 : 2);
    new_file->lens = size >> 10;
    new_file->fid = (uint16_t)reco->all_num; // 파일의 slot 인덱스 저장
    reco->all_num += 1;

    return new_file;
}

void fm_execute(FMv3_record *reco)
{
    // 1. 파일 관리자 초기화 상태 확인
    if (reco == NULL)
    {
        return; // 파일 관리자 미초기화
    }

    if (reco->base == NULL)
    {
        return; // 베이스 주소 미설정
    }

    // 2. 메모리 일관성 검증
    // 파일 개수와 마지막 주소 검증
    if (reco->all_num > (MAX_FCB_dir * MAX_FCB_file * MAX_FCB_file))
    {
        reco->all_num = 0; // 비정상적인 파일 개수 초기화
    }

    if (reco->cur_ptr > (MAX_FCB_dir * MAX_FCB_file * MAX_FCB_file))
    {
        reco->cur_ptr = 0; // 현재 포인터 초기화
    }

    // ! 시스템 동기화 (향후 구현)
    // ! fm_sync(reco);
}

uint32_t fm_write(FMv3_record *reco, int8_t path[27], void *buf, uint32_t size, uint32_t offset)
{

    fcb_t *file = fm_find(reco, path);
    uint32_t file_size;

    if (file == 0 || file->is_dir)
    {
        return 0;
    }

    file_size = (uint32_t)file->lens << 10;
    if (offset >= file_size)
    {
        return 0;
    }

    if (size > (file_size - offset))
    {
        size = file_size - offset;
    }

    memcpy((uint8_t *)fm_data_addr(reco, file) + offset, (uint8_t *)buf, size);
    return size;
}

// 파일 목록 조회
// 주어진 경로의 디렉토리 내 파일 목록을 출력
void fm_list(FMv3_record *reco, int8_t *path)
{
}

// 파일 찾기
fcb_t *fm_find(FMv3_record *reco, int8_t *path)
{
    uint16_t ret = search(reco->root, &path);

    if (ret != 0)
    {
        return &(reco->FMv3_mem[ret]);
    }

    return 0;
}

void *fm_data_addr(FMv3_record *reco, fcb_t *file)
{
    uint64_t base = (uint64_t)reco->base;
    uint64_t slot = (uint64_t)file->fid; // 파일의 slot 인덱스
    return (void *)(base + (slot * MAX_FILE_SIZE));
}

/*
    파일의 실제 저장 위치 계산

    레이아웃:
    - 메타데이터: FM_ADDR_START (FMv3_record 구조체)
    - 실제 데이터: USER_FILE_START

    슬롯 계산:
    - 파일 0: USER_FILE_START + (0 * MAX_FILE_SIZE)
    - 파일 1: USER_FILE_START + (1 * MAX_FILE_SIZE)
    - 파일 n: USER_FILE_START + (n * MAX_FILE_SIZE)

    각 파일은 MAX_FILE_SIZE(1MB)씩 떨어져 있음
*/