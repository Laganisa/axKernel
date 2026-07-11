#include "_fm.h"
#include "_io.h"
#include "_mm.h"
#include "_asm.h"
#include "_debug.h"

/*
    파일 만들기
    이름 입력하기 뭐 등등
    만들때 별칭은 8글자가 되도록 패딩을 해준다
    고로 글자는 전부 8글자이다(한국어 제외, 한국어는 3바이트를 쓰니까)
*/

//  init 함수
void fm_init(uint64_t *addr)
{
    fm_record->base = addr;
    fm_record->cur_ptr = 0;
    fm_record->all_num = 0;
    fm_record->last_addr = 0;
}

/*
    size는 B 단위로 받음
    최대 1MB (1048576 바이트)
    디렉토리당 최대 2MB (2097152 바이트)
    파일 생성 함수
*/
fcb_t *fm_create(FMv2_record *reco, int8_t path[27], uint32_t size, uint8_t ok_dir)
{
    int8_t normalized_path[27];
    normalize_path(path, normalized_path);
    path = normalized_path;

    // 확인: 1MB 초과 파일 거부
    if (size > MAX_FILE_SIZE)
    {
        return 0;
    }

    // 디렉토리인 경우 2MB 초과 거부
    if (ok_dir && size > MAX_DIR_SIZE)
    {
        return 0;
    }

    // 이름이 만들수 있는지 확인
    // 이름은 8글자여야 하고, '.'은 5번째 글자에만 올 수 있음

    for (int i = 0; i < 8; i++)
    {
        if (path[i] == 0x2E && i > 0x4) // '.' 이면서 뒤에 있으면
        {
            break; // or 리턴 만들수 없는 파일
        }
    }

    fcb_t *new_file = NULL; // 초기화 필수
    uint8_t top_addr = 16, mid_addr = 16, bot_addr = 16;
    uint8_t new_depth = 0;
    uint8_t auth; // 권한

    // 경로가 유효하다
    // 3. 경로 판별 및 할당 로직
    if (path[8] == 0x20) // [Case 1] 루트 직속 (Depth 0)
    {

        if (reco->last_addr >= 16)
        {

            return 0; // 공간 부족
        }
        if (fm_check(reco, 1, path) == FALSE)
        {

            return 0; // 중복 확인
        }
        uint8_t current_id = reco->last_addr;
        new_file = &(reco->FMv2_mem[current_id][16][16]);

        // 위치 정보 설정
        top_addr = current_id;
        new_depth = 0;

        // 관리 정보 업데이트
        reco->mapping[current_id][16][16] = token(path);
        reco->last_addr += 1;
    }
    else if (path[8] != 0x20 && path[17] == 0x20)
    {

        // 부모 디렉토리 인덱스 추출
        uint8_t pos_dir1 = token(&path[9]);
        fcb_t *parent_dir = &(reco->FMv2_mem[pos_dir1][16][16]);

        if (parent_dir->last_addr >= 16)
            return 0; // 부모 디렉토리 꽉 참
        if (fm_check(reco, 1, path) == FALSE)
            return 0; // 중복 확인

        uint8_t current_id = parent_dir->last_addr;
        new_file = &(reco->FMv2_mem[pos_dir1][current_id][16]);

        // 위치 정보 설정
        top_addr = pos_dir1;
        mid_addr = current_id;
        new_depth = 1;

        // 관리 정보 업데이트
        reco->mapping[pos_dir1][current_id][16] = token(path);
        parent_dir->last_addr += 1;
    }
    else if (path[17] != 0x20) // [Case 3] 3번째 경로 (Depth 2)
    {

        // ! Case 2와 유사하게 pos_dir1, pos_dir2 거쳐서 할당 로직 구현
        // top_addr = pos_dir1, mid_addr = pos_dir2, bot_addr = current_id
    }

    if (new_file == NULL)
    {
        return 0;
    }

    // 4. 새 파일/디렉토리 메타데이터 주입
    for (int i = 0; i < MAX_FILE_NAME; i++)
    {
        new_file->alias[i] = path[i];
    }

    new_file->is_dir = ok_dir;
    new_file->is_alloc = 1;
    new_file->depth = new_depth;
    new_file->lens = size >> 10; // KB 단위
    new_file->last_addr = 0;     // 새 디렉토리라면 자식 주소 0으로 초기화

    // 권한 및 위치 로직
    new_file->me_auth = 7;
    new_file->you_auth = 7;
    new_file->ppdir_addr = top_addr;
    new_file->pdir_addr = mid_addr;
    new_file->me_addr = bot_addr;

    reco->all_num += 1;

    return new_file; // 포인터 반환
}

void *fm_data_addr(FMv2_record *reco, fcb_t *file)
{
    uint64_t base = (uint64_t)reco->base;
    uint64_t slot = (uint64_t)slot_index(file);
    return (void *)(base + (slot * MAX_FILE_SIZE));
}

// 파일 쓰기
uint32_t fm_write(FMv2_record *reco, int8_t path[27], void *buf, uint32_t size, uint32_t offset)
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

/*
    파일 삭제
    주어진 경로의 파일을 삭제
    파일이 디렉토리인 경우 자식 파일이 없을 때만 삭제 가능
    ! 수정하기
*/
fcb_t *fm_delete(FMv2_record *reco, int8_t path[27])
{
    // 경로 유효성 검사
    if (fm_check(reco, 0, path) == FALSE)
    {
        return 0;
    }

    uint8_t top_addr = 0, mid_addr = 16, bot_addr = 16;
    fcb_t *target_file = NULL;
    uint16_t target_token = token(&path[18]); // 파일 이름의 토큰 값

    // 경로 깊이 판별
    if (path[8] == 0x20) // [Case 1] 루트 직속 (Depth 0)
    {
        // 루트 디렉토리에서 파일 검색
        for (int i = 0; i < 16; i++)
        {
            if (reco->mapping[i][16][16] == target_token)
            {
                target_file = &(reco->FMv2_mem[i][16][16]);
                top_addr = i;
                bot_addr = 16;
                break;
            }
        }
    }
    else if (path[8] != 0x20 && path[17] == 0x20) // [Case 2] 1단계 디렉토리
    {
        uint8_t pos_dir1 = token(&path[0]);

        // 첫 번째 디렉토리 내에서 파일 검색
        for (int i = 0; i < 16; i++)
        {
            if (reco->mapping[pos_dir1][i][16] == target_token)
            {
                target_file = &(reco->FMv2_mem[pos_dir1][i][16]);
                top_addr = pos_dir1;
                mid_addr = i;
                bot_addr = 16;
                break;
            }
        }
    }
    else if (path[17] != 0x20) // [Case 3] 2단계 디렉토리
    {
        uint8_t pos_dir1 = token(&path[0]);
        uint8_t pos_dir2 = token(&path[9]);

        // 두 번째 디렉토리 내에서 파일 검색
        for (int i = 0; i < 16; i++)
        {
            if (reco->mapping[pos_dir1][pos_dir2][i] == target_token)
            {
                target_file = &(reco->FMv2_mem[pos_dir1][pos_dir2][i]);
                top_addr = pos_dir1;
                mid_addr = pos_dir2;
                bot_addr = i;
                break;
            }
        }
    }

    if (target_file == NULL)
    {
        return 0; // 파일을 찾지 못함
    }

    // 디렉토리인 경우 자식 파일이 있는지 확인
    if (target_file->is_dir && target_file->last_addr > 0)
    {
        return 0; // 디렉토리에 파일이 있음 (삭제 불가)
    }

    // 파일/디렉토리 삭제
    target_file->is_alloc = 0; // 할당 해제

    // 파일 이름 초기화
    for (int i = 0; i < MAX_FILE_NAME; i++)
    {
        target_file->alias[i] = 0x00;
    }

    // 매핑 테이블 초기화
    if (bot_addr == 16 && mid_addr == 16) // 루트 직속
    {
        reco->mapping[top_addr][16][16] = 0x00;
    }
    else if (bot_addr == 16) // 1단계 디렉토리
    {
        reco->mapping[top_addr][mid_addr][16] = 0x00;
    }
    else // 2단계 디렉토리
    {
        reco->mapping[top_addr][mid_addr][bot_addr] = 0x00;
    }

    reco->all_num -= 1;

    return target_file;
}