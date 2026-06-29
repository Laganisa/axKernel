#include "fm.h"
#include "io.h"
#include "mm.h"
#include "asm.h"
#include "debug.h"

/*
    파일 간의 함수를 적는 파일
*/

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
                puts("\n"); // 줄바꿈 처리
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
                puts("\n"); //  줄바꿈 처리
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
                puts("\n"); // 줄바꿈 처리
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

    // 파일 개수 재계산 (오류 복구)
    if (valid_count != reco->all_num)
    {
        reco->all_num = valid_count;
    }

    // 매핑 테이블 검증
    // 루트 디렉토리의 매핑 테이블 일관성 확인
    for (int i = 0; i < reco->last_addr; i++)
    {
        if (reco->mapping[0][0][i] == 0x00)
        {
            // 매핑 테이블이 비어있으면, 해당 파일도 비활성화
            reco->FMv2_mem[0][0][i].is_alloc = 0;
        }
    }

    // ! 시스템 동기화 (향후 구현)
    // ! fm_sync(reco);
}

// 파일 찾기
fcb_t *fm_find(FMv2_record *reco, int8_t path[27])
{
    int8_t normalized_path[27];
    uint16_t target_token;

    normalize_path(path, normalized_path);
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

// 파일 실행
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

        if (proc == 0)
        {
            return 0;
        }

        real_addr = mm_find(&mm_stack, proc->mm_addr, 0);

        memcpy((uint8_t *)real_addr, ((uint8_t *)hdr) + sizeof(fm_exec_hdr_t), (uint32_t)hdr->image_size);

        entry_addr = real_addr + hdr->entry;
        proc->elr_el1 = entry_addr;

        return proc;
    }

    return 0;
}
