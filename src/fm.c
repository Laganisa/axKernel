#include "../include/defs.h"
#include "../include/fm.h"
#include "../include/io.h"
#include "../include/mm.h"

// init 함수

/*
void fm_init(uint64_t *addr)
{
    fmv2_record->base = addr;
    fmv2_record->cur_ptr = 0;
    fmv2_record->all_num = 0;
}
*/

// 파일 만들기
// 이름 입력하기 뭐 등등
// 만들때 별칭은 8글자가 되도록 패딩을 해준다
// 고로 글자는 전부 8글자이다(한국어 제외, 한국어는 3바이트를 쓰니까)

/*
   글자 8자 - 구문자 - 글자 8자 - 구문자 - 글자 8자
   해시함수에 넣기 좋게 자르면
   3글자 - 구문자 - 3글자 - 구문자
   3/3/3으로 쪼개고 차이를 저장하기
   매핑 테이블도 필요함
*/
// ! 수정 필요
uint16_t token(int8_t segment[8])
{
    // 1. 첫 글자 보정 (모음 -> 자음)
    int8_t first = segment[0];
    if (first == 65 || first == 69 || first == 73 || first == 79 || first == 85)
    {
        first -= 1;
    }

    // 2. 차이값 계산 (첫 번째 차이: 1글자-2글자, 두 번째 차이: 2글자-3글자)
    // 공백(0x20)이나 점('.')이 오면 차이를 0으로 처리하거나 예외처리
    int8_t diff1 = 0;
    int8_t diff2 = 0;

    if (segment[1] != 0x20 && segment[1] != '.')
    {
        diff1 = abs(segment[1] - segment[0]);
    }
    if (segment[2] != 0x20 && segment[2] != '.')
    {
        diff2 = abs(segment[2] - segment[1]);
    }

    // 3. 4비트 패킹 (하위 4비트만 추출하여 데이터 손실 방지)
    // 0xF(1111)와 AND 연산하여 4비트 범위를 강제함
    uint8_t packed_diffs = ((diff1 & 0x0F) << 4) | (diff2 & 0x0F);

    // 4. 최종 16비트 결과 생성 (첫 글자 8bit + 차이들 8bit)
    uint16_t result = (uint16_t)(first << 8) | packed_diffs;

    return result;
}

//  init 함수
void fm_init(uint64_t *addr)
{
    fm_record->base = addr;
    fm_record->cur_ptr = 0;
    fm_record->all_num = 0;
    fm_record->last_addr = 0;
}

static void fm_memcpy(uint8_t *dst, uint8_t *src, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

static void fm_normalize_path(const int8_t *src, int8_t dst[27])
{
    uint8_t i = 0;

    while (i < 26 && src[i] != '\0')
    {
        dst[i] = src[i];
        i++;
    }

    while (i < 26)
    {
        dst[i++] = 0x20;
    }

    dst[26] = '\0';
}

static uint32_t fm_slot_index(fcb_t *file)
{
    if (file->depth == 0)
    {
        return (uint32_t)file->ppdir_addr;
    }

    if (file->depth == 1)
    {
        return 16U + ((uint32_t)file->ppdir_addr * 16U) + (uint32_t)file->pdir_addr;
    }

    return 16U + 256U + ((uint32_t)file->ppdir_addr * 256U) + ((uint32_t)file->pdir_addr * 16U) + (uint32_t)file->me_addr;
}

// size는 B 단위로 받음
// 최대 1MB (1048576 바이트)
// 디렉토리당 최대 2MB (2097152 바이트)
// 파일 생성 함수
/*
    뭐 적기
*/
fcb_t *fm_create(FMv2_record *reco, int8_t path[27], uint32_t size, uint8_t ok_dir)
{
    puts("entering fm_creat\n");
    int8_t normalized_path[27];
    fm_normalize_path(path, normalized_path);
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

    puts("checking path\n");

    puts("checking path\n");

    // 경로가 유효하다
    // 3. 경로 판별 및 할당 로직
    if (path[8] == 0x20) // [Case 1] 루트 직속 (Depth 0)
    {
        puts("checking path\n");

        if (reco->last_addr >= 16)
        {
            return 0; // 공간 부족
        }
        if (fm_check(reco, 1, path) == FALSE)
        {
            puts("checking path mul\n");
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
        puts("checking path\n");
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
        return 0;

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

    puts("checking path\n");

    reco->all_num += 1;

    return new_file; // 포인터 반환
}

fcb_t *fm_find(FMv2_record *reco, int8_t path[27])
{
    int8_t normalized_path[27];
    uint16_t target_token;

    fm_normalize_path(path, normalized_path);
    path = normalized_path;

    if (path[8] == 0x20)
    {
        target_token = token(&path[0]);
        for (int i = 0; i < 16; i++)
        {
            if (reco->mapping[i][16][16] == target_token)
            {
                return &(reco->FMv2_mem[i][16][16]);
            }
        }
    }
    else if (path[8] != 0x20 && path[17] == 0x20)
    {
        uint8_t pos_dir1 = token(&path[0]);
        target_token = token(&path[9]);

        for (int i = 0; i < 16; i++)
        {
            if (reco->mapping[pos_dir1][i][16] == target_token)
            {
                return &(reco->FMv2_mem[pos_dir1][i][16]);
            }
        }
    }
    else if (path[17] != 0x20)
    {
        uint8_t pos_dir1 = token(&path[0]);
        uint8_t pos_dir2 = token(&path[9]);
        target_token = token(&path[18]);

        for (int i = 0; i < 16; i++)
        {
            if (reco->mapping[pos_dir1][pos_dir2][i] == target_token)
            {
                return &(reco->FMv2_mem[pos_dir1][pos_dir2][i]);
            }
        }
    }

    return 0;
}

void *fm_data_addr(FMv2_record *reco, fcb_t *file)
{
    uint64_t base = (uint64_t)reco->base;
    uint64_t slot = (uint64_t)fm_slot_index(file);
    return (void *)(base + (slot * MAX_FILE_SIZE));
}

uint32_t fm_write(FMv2_record *reco, int8_t path[27], void *buf, uint32_t size, uint32_t offset)
{
    puts("b");
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

    fm_memcpy((uint8_t *)fm_data_addr(reco, file) + offset, (uint8_t *)buf, size);
    return size;
}

pcb_t *fm_exec_file(FMv2_record *reco, PMv1_object *obj, int8_t path[27], uint8_t parid)
{
    fcb_t *file = fm_find(reco, path);
    fm_exec_hdr_t *hdr;

    if (file == 0 || file->is_dir)
    {
        return 0;
    }

    hdr = (fm_exec_hdr_t *)fm_data_addr(reco, file);
    if (hdr->magic != FM_EXEC_MAGIC)
    {
        return 0;
    }

    if (hdr->mode == FM_EXEC_MODE_DIRECT)
    {
        return creat_proc_entry(obj, hdr->entry, parid);
    }

    if (hdr->mode == FM_EXEC_MODE_IMAGE)
    {
        pcb_t *proc = creat_proc_entry(obj, 0, parid);
        uint64_t real_addr;
        uint64_t entry_addr;
        uint64_t *reg_val;

        if (proc == 0)
        {
            return 0;
        }

        real_addr = mm_find(&mm_stack, proc->mm_addr, 0);
        fm_memcpy((uint8_t *)real_addr, ((uint8_t *)hdr) + sizeof(fm_exec_hdr_t), (uint32_t)hdr->image_size);

        entry_addr = real_addr + hdr->entry;
        proc->pc = entry_addr;
        reg_val = (uint64_t *)proc->sp;
        reg_val[32] = entry_addr;
        return proc;
    }

    return 0;
}

// 파일 삭제
// 주어진 경로의 파일을 삭제
// 파일이 디렉토리인 경우 자식 파일이 없을 때만 삭제 가능
// ! 수정하기
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

// 파일 목록 조회
// 주어진 경로의 디렉토리 내 파일 목록을 출력
void fm_list(FMv2_record *reco, int8_t path[27])
{
    // 경로 유효성 검사
    if (fm_check(reco, 0, path) == FALSE)
    {
        return;
    }

    uint8_t top_addr = 0, mid_addr = 16, bot_addr = 16;
    fcb_t *target_dir = NULL;

    // 경로 깊이 판별
    if (path[8] == 0x20) // [Case 1] 루트 디렉토리
    {
        // 루트 디렉토리의 파일들을 나열
        for (int i = 0; i < reco->last_addr; i++)
        {
            fcb_t *current_file = &(reco->FMv2_mem[i][16][16]);

            // 할당된 파일만 표시
            if (current_file->is_alloc)
            {
                // 파일 정보 출력
                // 1. 파일 이름 출력
                for (int j = 0; j < MAX_FILE_NAME; j++)
                {
                    if (current_file->alias[j] != 0x00 && current_file->alias[j] != 0x20)
                    {
                        putchar(current_file->alias[j]); // %c 대신 문자 출력 함수 사용
                    }
                }

                // 2. 타입 및 크기 출력 (Hex 버전)
                puts(" [");
                if (current_file->is_dir)
                {
                    puts("DIR");
                }
                else
                {
                    puts("FILE");
                }
                puts("] 0x");

                put_hex(current_file->lens); // 10진수 %u 대신 hex로 출력
                puts("KB");
                puts("\n"); // puts(\n)이 된다고 했으니 줄바꿈 처리
            }
        }
    }
    else if (path[8] != 0x20 && path[17] == 0x20) // [Case 2] 1단계 디렉토리
    {
        uint8_t pos_dir1 = token(&path[0]);
        target_dir = &(reco->FMv2_mem[pos_dir1][16][16]);

        // 디렉토리 확인
        if (!target_dir->is_dir)
        {
            return; // 디렉토리가 아님
        }

        // 해당 디렉토리의 자식 파일들을 나열
        for (int i = 0; i < target_dir->last_addr; i++)
        {
            fcb_t *current_file = &(reco->FMv2_mem[pos_dir1][i][16]);

            // 할당된 파일만 표시
            if (current_file->is_alloc)
            {
                // 파일 정보 출력
                // 1. 파일 이름 출력
                for (int j = 0; j < MAX_FILE_NAME; j++)
                {
                    if (current_file->alias[j] != 0x00 && current_file->alias[j] != 0x20)
                    {
                        putchar(current_file->alias[j]); // %c 대신 문자 출력 함수 사용
                    }
                }

                // 2. 타입 및 크기 출력 (Hex 버전)
                puts(" [");
                if (current_file->is_dir)
                {
                    puts("DIR");
                }
                else
                {
                    puts("FILE");
                }
                puts("] 0x");

                put_hex(current_file->lens); // 10진수 %u 대신 hex로 출력
                puts("KB");
                puts("\n"); // puts(\n)이 된다고 했으니 줄바꿈 처리
            }
        }
    }
    else if (path[17] != 0x20) // [Case 3] 2단계 디렉토리
    {
        uint8_t pos_dir1 = token(&path[0]);
        uint8_t pos_dir2 = token(&path[9]);
        target_dir = &(reco->FMv2_mem[pos_dir1][pos_dir2][16]);

        // 디렉토리 확인
        if (!target_dir->is_dir)
        {
            return; // 디렉토리가 아님
        }

        // 해당 디렉토리의 자식 파일들을 나열
        for (int i = 0; i < target_dir->last_addr; i++)
        {
            fcb_t *current_file = &(reco->FMv2_mem[pos_dir1][pos_dir2][i]);

            // 할당된 파일만 표시
            if (current_file->is_alloc)
            {
                // 파일 정보 출력
                // 1. 파일 이름 출력
                for (int j = 0; j < MAX_FILE_NAME; j++)
                {
                    if (current_file->alias[j] != 0x00 && current_file->alias[j] != 0x20)
                    {
                        putchar(current_file->alias[j]); // %c 대신 문자 출력 함수 사용
                    }
                }

                // 2. 타입 및 크기 출력 (Hex 버전)
                puts(" [");
                if (current_file->is_dir)
                {
                    puts("DIR");
                }
                else
                {
                    puts("FILE");
                }
                puts("] 0x");

                put_hex(current_file->lens); // 10진수 %u 대신 hex로 출력
                puts("KB");
                puts("\n"); // puts(\n)이 된다고 했으니 줄바꿈 처리
            }
        }
    }
}

// 파일 관리자 실행 및 상태 검증
// 시스템 초기화 상태 확인, 메모리 일관성 검증, 복구 작업 수행
void fm_execute(FMv2_record *reco)
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

    if (reco->last_addr > 16) // 루트 디렉토리 최대 16개 파일
    {
        reco->last_addr = 0;
    }

    if (reco->cur_ptr > (MAX_FCB_dir * MAX_FCB_file * MAX_FCB_file))
    {
        reco->cur_ptr = 0; // 현재 포인터 초기화
    }

    // 3. 루트 디렉토리 메타데이터 검증
    // 할당된 파일들의 상태 확인
    uint8_t valid_count = 0;
    for (int i = 0; i < reco->last_addr; i++)
    {
        fcb_t *current_fcb = &(reco->FMv2_mem[0][0][i]);

        // 할당된 파일인지 확인
        if (current_fcb->is_alloc)
        {
            // 파일 이름 유효성 검사
            if (current_fcb->alias[0] != 0x00 && current_fcb->alias[0] != 0x20)
            {
                valid_count++;
            }
            else
            {
                // 비정상적인 파일, 할당 해제 표시
                current_fcb->is_alloc = 0;
            }
        }
    }

    // 4. 파일 개수 재계산 (오류 복구)
    if (valid_count != reco->all_num)
    {
        reco->all_num = valid_count;
    }

    // 5. 매핑 테이블 검증
    // 루트 디렉토리의 매핑 테이블 일관성 확인
    for (int i = 0; i < reco->last_addr; i++)
    {
        if (reco->mapping[0][0][i] == 0x00)
        {
            // 매핑 테이블이 비어있으면, 해당 파일도 비활성화
            reco->FMv2_mem[0][0][i].is_alloc = 0;
        }
    }

    // 6. 시스템 동기화 (향후 구현)
    // fm_sync(reco);
}

//  경로 유효성 검사
/*
    규칙에 따라 경로가 유효한지 확인
    cmd == 0
    1. 구문자가 고정된 위치가 맞는지 검사
    2. 각 마디에 '.' 의 위치가 맞는지 검사
    3. 공백 후 문자가 나오는지 검사
    cmd == 1
    1. 경로에서 최근 디렉토리를 알아낸다
    2. 디랙토리를 알아냈다면 mapping 테이블에서
    현재 주소를 변환한 값(저기 위에 token 함수를 통과한 값)이랑
    같은 문자가 있는지 확인
*/
bool fm_check(FMv2_record *reco, uint8_t cmd, int8_t path[27])
{
    puts("entering fm_check\n");

    // 경로길이가 짧으면 늘려주는 함수
    // 얼마나 짧은지 확인
    // 확인 후 구분자 및 널 문자를 넣어줌
    uint8_t i = 0;

    // 문자열 끝 찾기
    while (i < 26 && path[i] != '\0')
    {
        i++;
    }

    // 끝까지 공백 채우기
    while (i < 26)
    {
        path[i++] = 0x20;
    }

    // 마지막 널 문자
    path[26] = '\0';
    // while 문으로 길이를 확인
    for (int i = 0; i < 27; i++)
    {
        // 다음 문자가 널 문자면
        if (path[i] == "\0")
        {
            // 늘려주고
            break;
        }
    }

    // 경로 유호성 검사
    if (cmd == 0)
    {
        bool is_ok = TRUE;

        // 1. 구문자 및 널 문자 고정 위치 검사
        if (path[8] != '/' || path[17] != '/' || path[26] != '\0')
        {
            puts("spe\n");
            return FALSE;
        }
        if (path[22] != '.')
        {
            puts("rty\n");
            return FALSE; // 파일명 영역의 '.' 위치 고정
        }
        // 2. 이중 루프로 각 마디(8글자씩) 검사
        int segments[3] = {0, 9, 18}; // 각 마디의 시작 인덱스

        for (int s = 0; s < 3; s++)
        {
            int start = segments[s];
            bool space_started = FALSE;

            for (int i = 0; i < 8; i++)
            {
                int8_t current_char = path[start + i];

                // 파일명 마디(s==2)에서 '.' 위치(인덱스 22)는 건너뜀
                if (s == 2 && (start + i) == 22)
                {
                    continue;
                }

                if (current_char == 0x20)
                {
                    space_started = TRUE; // 이제부터는 계속 공백이어야 함
                }
                else
                {
                    // 공백이 이미 시작됐는데 문자가 나타나면 규칙 위반
                    if (space_started)
                    {
                        is_ok = FALSE;
                        puts("q\n");
                        break;
                    }
                    // ! 특수 문자 로직 넣는 자리
                    // 보통은 특수 문자를 허용하지 않음
                }
            }
            if (!is_ok)
            {
                puts("w\n");
                break;
            }
        }
        puts("e\n");
        return is_ok;
    }
    // 디랙토리에 중복 있는지 검사
    else if (cmd == 1)
    {
        // 검사하는 법
        /*
            경로에서 최근 디렉토리를 알아낸다
            디랙토리를 알아냈다면 mapping 테이블에서
            현재 주소를 변환한 값(저기 위에 token 함수를 통과한 값)이랑
            같은 문자가 있는지 확인
            확인은 일단 저 mapping 테이블이 8비트여서 문자 8개씩 비교할 수 있음
            또한 한 디렉토리당 파일 수가 16개 여서 2번 반복하면 됨
        */
        // 그럼 일단 현재 경로에서 위에 디렉토리가 몇개인지 확인하는 로직
        // 경로에서 앞의 문자가 공백이 아니면 디렉토리가 있다 판단
        // 항상 경로를 만들때 앞에서 부터 채우기 때문
        // 8번쨰는 구문자여서
        // 9번째와 18번째 글자가 공백이 아니면 각각 디렉토리가 있다고 판단

        uint8_t dir_count = 0;
        uint8_t pos_dir1 = 0;
        uint8_t pos_dir2 = 0;

        if (path[9] != 0x20)
        {
            // 그 앞을 pos_dir1로 저장
            pos_dir1 = token(&path[0]);
            dir_count++;
        }
        if (path[18] != 0x20)
        {
            // 그 앞을 pos_dir2로 저장
            pos_dir2 = token(&path[9]);
            dir_count++;
        }

        bool is_ok = TRUE;

        // 디렉토리는 다음에 나오는 위치가 16임
        if (dir_count == 0)
        {
            // 루트 디렉토리인 경우
            // 매핑 테이블에서 루트 디렉토리에 해당하는 값이 있는지 확인
            // ! 간단한 구현으로 대체
            for (int i = 0; i < 16; i++)
            {
                if (reco->mapping[i][16][16] == token(path))
                {
                    // 값이 존재함
                    is_ok = FALSE;
                    break;
                }
            }
        }
        else if (dir_count == 1)
        {
            // 첫번째 디렉토리가 있는 경우
            // 매핑 테이블에서 첫번째 디렉토리에 해당하는 값이 있는지 확인

            // ! 간단한 구현으로 대체
            for (int i = 0; i < 16; i++)
            {
                if (reco->mapping[pos_dir1][i][16] == token(path))
                {
                    // 값이 존재함
                    is_ok = FALSE;
                    break;
                }
            }
        }
        else if (dir_count == 2)
        {
            // 두번째 디렉토리가 있는 경우
            // 매핑 테이블에서 두번째 디렉토리에 해당하는 값이 있는지 확인

            // ! 간단한 구현으로 대체
            for (int i = 0; i < 16; i++)
            {
                if (reco->mapping[pos_dir1][pos_dir2][i] == token(path))
                {
                    // 값이 존재함
                    is_ok = FALSE;
                    break;
                }
            }
        }

        if (is_ok)
        {
            // 만약 중복이 없다면
            return TRUE;
        }
        else
        {
            // 중복이 있다면
            return FALSE;
        }
    }
    // 맞지 않는 명령
    else
    {
        return FALSE;
    }
}

/*
fcb_t *fm_delete_(FMv2_record *reco, uint8_t path[27]) {}

fcb_t *fm_find_path(FMv2_record *reco, uint8_t path[27]) {}

void fm_list_dir(FMv2_record *reco, uint8_t path[27]) {}

int32_t fm_read_(FMv2_record *reco, uint8_t path[27], uint64_t offset, uint8_t *buf, uint32_t size) {}

int32_t fm_write_(FMv2_record *reco, uint8_t path[27], uint64_t offset, uint8_t *buf, uint32_t size) {}

int8_t fm_rename_(FMv2_record *reco, uint8_t path[27], int8_t new_name[8]) {}

int8_t fm_change_auth_(FMv2_record *reco, uint8_t path[27], uint8_t me_auth, uint8_t you_auth) {}

fcb_t *fm_get_info_(FMv2_record *reco, uint8_t path[27]) {}

uint32_t fm_allocate_space_(FMv2_record *reco, uint32_t size) {}

void fm_deallocate_space_(FMv2_record *reco, uint32_t addr, uint32_t size) {}

int8_t fm_mkdir_(FMv2_record *reco, uint8_t path[27], int8_t dir_name[8]) {}

int8_t fm_rmdir_(FMv2_record *reco, uint8_t path[27]) {}

uint8_t fm_check_available_(FMv2_record *reco, uint32_t required_size) {}

void fm_sync_(FMv2_record *reco) {}
*/
