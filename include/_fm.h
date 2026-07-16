// #include "defs.h"
#include "_pm.h"
#include "_sect.h"
#include "_defs.h"

#ifndef __FM_H__
#define __FM_H__

typedef struct fcb_t
{
    // 파일 이름 (8 bytes)
    // /0 또한 포함
    int8_t alias[MAX_FILE_NAME + 1];

    uint16_t lens : 11; // 파일 길이 (1KB 단위, 최대 1MB)
    uint16_t fid : 5;   // 파일 id

    uint16_t depth : 3;      // 파일 깊이 (0~2)
    uint16_t is_dir : 1;     // 디렉토리 여부
    uint16_t me_auth : 3;    // 나의 권한
    uint16_t team_auth : 3;  // 너의 권한
    uint16_t other_auth : 3; // 타인의 권한
    uint16_t is_alloc : 1;   // 할당 여부
    uint16_t is_lock : 1;    // 누가 읽고 있는지 확인
    uint16_t padding : 1;

    uint8_t checksum; // 체크섬

    uint16_t YYYY : 7;
    uint16_t MM : 4;
    uint16_t DD : 5;

} fcb_t;

// 파일 관리자 구조체 V2
typedef struct FMv3_record
{
    uint64_t *base;        // 바닥 주소
    uint16_t cur_ptr : 16; // 보고 있는 주소 읽을때 씀
    uint16_t all_num;
    struct bpt_node *root;

    // 메타데이터 배열
    fcb_t FMv3_mem[MAX_FILE_NUM];
} FMv3_record;

typedef struct fm_exec_hdr_t
{
    uint64_t magic;
    uint64_t mode;
    uint64_t entry;
    uint64_t image_size;
} fm_exec_hdr_t;

typedef struct bpt_node
{
    uint8_t data_num : 7;
    uint8_t leaf : 1;
    uint64_t fnv1a_hash_key[MAX_BPT_NODE_NUM - 1]; // 검색에 주된 키
    uint64_t djb2_hash_Key[MAX_BPT_NODE_NUM - 1];  // 다시 확인용도
    uint16_t data[MAX_BPT_NODE_NUM - 1];           // 3개
    struct node *parent;                           // 부모 포인터
    struct node *child[MAX_BPT_NODE_NUM];
    struct node *next;
} bpt_node;

#pragma region fmv2

#define fm_record ((FMv3_record *)FM_ADDR_START)

void fm_init(uint64_t *addr);
fcb_t *fm_create(FMv3_record *reco, int8_t *path, uint32_t size, uint8_t ok_dir);
fcb_t *fm_delete(FMv3_record *reco, int8_t path[27]);
fcb_t *fm_find(FMv3_record *reco, int8_t *path);
void *fm_data_addr(FMv3_record *reco, fcb_t *file);
uint32_t fm_write(FMv3_record *reco, int8_t path[27], void *buf, uint32_t size, uint32_t offset);
void fm_list(FMv3_record *reco, int8_t path[27]);
void fm_execute(FMv3_record *reco);

#pragma endregion

#pragma region fmv3

bpt_node *search_leaf(bpt_node *root, uint64_t key);
bpt_node *create_node(uint8_t leaf);
void insert(bpt_node *root, char *name, uint16_t value);
uint16_t *search(bpt_node *root, char *name);

#pragma endregion

#endif
