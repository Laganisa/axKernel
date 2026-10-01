#ifndef __KERNEL_FM_H__
#define __KERNEL_FM_H__

#include "manage/_pm.h"
#include "_sect.h"
#include "_defs.h"

// 파일 제어 블럭 타입
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

    uint16_t YYYY : 7;
    uint16_t MM : 4;
    uint16_t DD : 5;

    uint8_t checksum; // 체크섬

} fcb_t;

// 디렉토리 제어 블럭 엔트리
typedef struct fcb_e
{
    // 디렉토리 이름 (8 bytes)
    // /0 또한 포함
    int8_t alias[MAX_FILE_NAME + 1];

    // 디렉토리 안에 있는 파일 id
    uint16_t files[MAX_DIR_FILE_NUM];

    uint16_t p_fid : 6; // 부모 디렉토리 id
    uint16_t fid : 6;   // 디렉토리 id
    uint16_t depth : 4; // 디렉토리 깊이 (0~2)

    uint16_t auth : 10;   // 권한
    uint16_t padding : 6; // 패딩

    uint16_t YYYY : 7;
    uint16_t MM : 4;
    uint16_t DD : 5;

    uint8_t checksum; // 체크섬

} fcb_e;

/// @brief 파일 관리자 구조체 V3
typedef struct FMv3_record
{
    uint64_t *base;   // 바닥 주소
    uint16_t cur_ptr; // 보고 있는 주소 읽을때 씀(아직 쓰지 않음)

    uint16_t all_file_num; // 전체 파일 수
    uint16_t all_dir_num;  // 전체 디렉토리 수

    // bpt의 루트 노드
    struct bpt_node *root;

    // bpt의 디렉토리 루트 노드
    struct bpt_node *dir_root;

    /*
        메타 데이터 배열
        동적할당을 생각중이긴 함
    */
    struct fcb_t FMv3_mem[MAX_FILE_NUM];    // 파일 메타데이터 배열
    struct fcb_e FMv3_dir_mem[MAX_DIR_NUM]; // 디렉토리 메타데이터 배열

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
void fm_execute(FMv3_record *reco);
fm_exec_hdr_t *fm_data_addr(FMv3_record *reco, fcb_t *file);

fcb_t *fm_find(FMv3_record *reco, char *name);
void fm_list(FMv3_record *reco, int8_t *path);

// 파일 생성 & 삭제
fcb_t *fm_create(
    FMv3_record *reco,
    char *path,
    uint32_t size,
    uint16_t auth);

uint8_t fm_delete(
    FMv3_record *reco,
    char *path);

// 파일 쓰기 & 읽기
uint32_t fm_write(
    FMv3_record *reco,
    fcb_t *file,
    void *buf,
    uint32_t size,
    uint32_t offset);

uint32_t fm_read(
    FMv3_record *reco,
    fcb_t *file,
    void *buf,
    uint32_t size,
    uint32_t offset);

// 파일 열기
// fcb_t *fm_open(void);
// 파일 닫기

// 디렉토리 생성 & 삭제
fcb_e *fm_dir_create(
    FMv3_record *reco,
    char *name,
    uint32_t p_fid,
    uint16_t depth,
    uint16_t auth);

uint8_t fm_dir_delete(FMv3_record *reco, char *name);

// 나중에 스태틱으로 박을 예정
uint8_t fm_dir_in(
    FMv3_record *reco,
    fcb_e *dir,
    uint16_t file_id);

uint8_t fm_dir_out(
    FMv3_record *reco,
    fcb_e *dir,
    uint16_t file_id);

#endif
