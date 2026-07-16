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

    uint16_t depth : 4;    // 파일 깊이 (0~2)
    uint16_t auth : 10;    // 권한
    uint16_t is_alloc : 1; // 할당 여부
    uint16_t is_lock : 1;  // 누가 읽고 있는지 확인

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

#define fm_record ((FMv3_record *)FM_ADDR_START)

void fm_init(uint64_t *addr);
fcb_t *fm_create(FMv3_record *reco, char *path, uint32_t size, uint16_t auth);
fcb_t *fm_delete(FMv3_record *reco, char *path);
fcb_t *fm_find(FMv3_record *reco, char *name);
void *fm_data_addr(FMv3_record *reco, fcb_t *file);
uint32_t fm_write(FMv3_record *reco, char *name, void *buf, uint32_t size, uint32_t offset);
void fm_list(FMv3_record *reco, int8_t *path);
void fm_execute(FMv3_record *reco);

#endif
