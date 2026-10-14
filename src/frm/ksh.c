#include "global/_in_proc.h"
#include "global/_io.h"
#include "tools/_asm.h"
#include "global/_debug.h"
#include "tools/_font.h"
#include "tools/_dstruc.h"
#include "tools/_hash.h"

enum shell_ret
{
    COMM_OK = 0,
    COMM_ERR,

    UNKNOW_ERR,
    GRMMER_ERR,

    ARGS_ERR,
};

enum shell_incode
{
    PIPE_INCO = 1641,
    BACK_INCO = 1477,
    REDI_INCO = 1621,
    SCRI_INCO = 1649,
};

// 현재 작업 경로를 나타내는 로직
// 스택에서 요소씩 꺼내고 출력
static void kpath()
{
    // 꺼내고 출력 반복
    while (1)
    {
    }
}

// 메모리
static uint8_t mem_split(char *args)
{
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    /*
    switch (hash_check)
    {
    // 힙 공간 보기
    case:

        break;
        // 페이지 테이블 보기
    case:

        break;
        // 프로새스 정보
    case:

        break;
        // 프로세스 상태(생명주기) 변경
    case:

        break;

    default:
        ret = COMM_ERR;
        break;
    }
*/
    return ret;
}

// 프로세스
static uint8_t proc_split(char **args)
{
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    /*
    switch (hash_check)
    {
        // 프로세스 생성
    case:

    break;
    // 프로세스 삭제
case:

    break;
    // 프로새스 정보
case:

    break;
    // 프로세스 상태(생명주기) 변경
case:

    break;

default:
    ret = COMM_ERR;
    break;
}
    */
    return ret;
}

// 파일
/*
    file {명령}

    crt
    del
    info

    dir_crt
    dir_del
    dir_info

    disk_crt
*/

// 디렉토리랑 현재 깊이를 전달 받게 수정
static uint8_t file_split(char **args)
{
    uint8_t ret;

    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    fcb_t *now_fcb = NULL;
    fcb_e *now_dir_fcb = NULL;

    switch (hash_check)
    {
    // 파일 생성 명령
    case 0xF5A2F5190CAEA09C:
        targs[2] = str_to_uint(args[2]);
        targs[3] = str_to_uint(args[3]);
        targs[4] = str_to_uint(args[4]);
        targs[5] = str_to_uint(args[5]);

        // 예외 처리
        // 생성파일의 크기가 너무 작거나 클때
        if (targs[4] | targs[5] != 0)
        {
            puts("form : file crt {name} {size(B)} {auth}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_create(
            fm_record,
            args[1],
            (uint32_t)targs[2],
            (uint16_t)targs[3]);

        if (now_fcb == NULL)
        {
            ret = COMM_ERR;
        }
        else
        {
            ret = COMM_OK;
        }
        break;

    // 파일 삭제
    case 0xCA9A2418F461F5CA:
        fm_delete(fm_record, args[1]);
        ret = COMM_OK;
        break;

    // 파일 정보
    case 0xf5d6a8c57ab94f05:
        now_fcb = fm_find(fm_record, args[1]);

        if (now_fcb == NULL)
        {
            puts("file not found\n");
            ret = COMM_ERR;
            break;
        }

        puts("file name : ");
        puts(now_fcb->alias);
        puts("\n");
        puts("file len(KB): ");
        put_uint((uint64_t)now_fcb->lens);
        puts("\n");
        puts("file id: ");
        put_uint((uint64_t)now_fcb->fid);
        puts("\n");
        puts("file auth : ");
        put_uint((uint64_t)now_fcb->auth);
        puts("\n");

        puts("YYYY : ");
        put_uint((uint64_t)now_fcb->YYYY);
        puts("  MM : ");
        put_uint((uint64_t)now_fcb->MM);
        puts("  DD : ");
        put_uint((uint64_t)now_fcb->DD);
        puts("\n");
        break;

    // 파일의 디렉토리간 이동
    case 0xF5FAF8190CF91B8B:
        break;
    // 파일 권한 수정
    case 0x5DC77A84260A774F:
        break;

    // 디렉토리 생성
    case 0x2268A5C380EEAA08:
        targs[2] = str_to_uint(args[2]);
        targs[3] = str_to_uint(args[3]);
        targs[4] = str_to_uint(args[4]);

        // 예외 처리
        // 생성파일의 크기가 너무 작거나 클때
        if (0)
        {
            puts("form : file dir_crt {name} {size(B)} {auth}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_dir_create(
            fm_record,
            args[1],
            (uint32_t)targs[2],
            (uint32_t)targs[3],
            (uint16_t)targs[4]);

        if (now_fcb == NULL)
        {
            ret = COMM_ERR;
        }
        else
        {
            ret = COMM_OK;
        }
        break;

    // 디렉토리 삭제
    case 0x3BB72CC38F126296:
        fm_dir_delete(fm_record, args[1]);
        ret = COMM_OK;
        break;

        // 디렉토리 정보
    case 0x04B54668DC76A7A9:
        now_dir_fcb = fm_dir_find(fm_record, args[1]);

        if (now_dir_fcb == NULL)
        {
            ret = COMM_ERR;
            break;
        }

        puts("dir name : ");
        puts(now_dir_fcb->alias);
        puts("\n");

        puts("dir id: ");
        put_uint((uint64_t)now_dir_fcb->fid);
        puts("\n");
        puts("dir auth : ");
        put_uint((uint64_t)now_dir_fcb->auth);
        puts("\n");

        if (now_dir_fcb->dir_num == 0)
        {
            puts("file not found\n");
        }
        else
        {
            puts("files\n");
            for (int i = 0; i < now_dir_fcb->dir_num; i++)
            {
                char *names = &(fm_record->FMv3_mem[now_dir_fcb->files[i]]).alias;
                puts(names);
                puts("\n");
            }
        }

        puts("YYYY : ");
        put_uint((uint64_t)now_dir_fcb->YYYY);
        puts("  MM : ");
        put_uint((uint64_t)now_dir_fcb->MM);
        puts("  DD : ");
        put_uint((uint64_t)now_dir_fcb->DD);
        puts("\n");
        break;

    // 디렉토리의 디렉토리간 이동
    case 0x22C0B8C381394027:
        break;

    // 디렉토리 권한 수정
    case 0x36AA60224FAA08BB:
        break;

        // 나중에 디스크 관리
        /*

        */

    default:
        ret = COMM_ERR;
        break;
    }

    return ret;
}

// 장치
static uint8_t dev_split(char *args)
{
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    /*
    switch (hash_check)
    {
        // 장치 추가
    case:

        break;
        // 장치 삭제
    case:

        break;
        // 장치 정보
    case:

        break;
        // 장치 열기
    case:

        break;
        // 장치 닫기
    case:

        break;
    default:
        ret = COMM_ERR;
        break;
    }
*/
    return ret;
}

// 시스템 처리
// 사실 잡다한 것들 용도
static uint8_t sys_split(char *args)
{
    uint8_t ret;

    return ret;
}

// 프롬포트를 분리하는 로직
static void ksplit(char *args)
{
    // 인코딩 값
    uint32_t incode = (uint32_t)args[0];

    // 제어 변수
    uint8_t con = 0b1111;

    // 처리로직
    int j = 1;

check:

    int i = j;

    for (; i < 5; i++)
    {
        // 공백이 나오면 그만 하기
        if (args[i] == 0x20)
        {
            break;
        }

        incode = (incode << 1) + (uint32_t)args[i];
    }

    // 만약 제어 명령이라면?
    switch (incode)
    {
    // 파이프라인
    case PIPE_INCO:
        con = 0b0000;
        j = i;
        goto check;

    // 백그라운드
    case BACK_INCO:
        con = 0b0000;
        j = i;
        goto check;

    // 리디렉션
    case REDI_INCO:
        con = 0b0000;
        j = i;
        goto check;

    // 스크립트
    case SCRI_INCO:
        con = 0b0000;
        j = i;
        goto check;

    // 아무 제어 명령이 없다면?
    default:
        // 아무것도 안하기
        break;
    }

    // char * 형 배열 준비 (최대 6개)
    char *command[6] = {0};
    int temp_idx = 0;

    // 위쪽 파싱에서 멈춘 위치 i부터 시작
    while (args[i] != '\0' && temp_idx < 6)
    {
        // 공백이나 쉼표 등 구분자 건너뛰기
        while (args[i] == ' ' || args[i] == ',')
        {
            args[i] = '\0'; // 원본 문자열을 널 문자로 끊어줌 (선택 사항이지만 안전함)
            i++;
        }

        if (args[i] == '\0')
        {
            break;
        }

        // 토큰의 시작 주소를 포인터 배열에 저장
        command[temp_idx++] = &args[i];

        // 다음 공백이나 널 문자가 나올 때까지 이동
        while (args[i] != '\0' && args[i] != ' ' && args[i] != ',')
        {
            i++;
        }
    }

#pragma endregion

    // 리턴 변수 받는 부분
    uint8_t ret = UNKNOW_ERR;

    // 메모리
    if (incode == 747 && (con & 0b1))
    {
        ret = mem_split(command);
    }
    // 프로세스
    else if (incode == 1673 &&
             ((con >> 1 & 0b1)))
    {
        ret = proc_split(command);
    }
    // 파일
    else if (incode == 1553 &&
             ((con >> 2) & 0b1))
    {
        ret = file_split(command);
    }
    // 장치
    else if (incode == 1673 &&
             ((con >> 3) & 0b1))
    {
        ret = dev_split(command);
    }
    // 시스템
    else if (incode == 817)
    {
        ret = sys_split(command);
    }

    // ret에 맞는 처리
    switch (ret)
    {
    // 잘못된 범주 선택
    case COMM_OK:
        break;

    case UNKNOW_ERR:
        puts("Invalid category selected\n");
        break;

    // 문법 오류
    case GRMMER_ERR:
        puts("Grammar error\n");
        break;

    // 인자 전달 오류
    case ARGS_ERR:
        puts("Invalid arguments\n");
        break;

    // 명령 오류
    case COMM_ERR:
        puts("Command error\n");
        break;
    // 그 외 예상치 못한 오류
    default:
        puts("Unexpected error\n");
        break;
    }
}

// 기초적인 커널 내장 쉘
void kshell(void)
{
    disable_irq();

    // 경로 설정
    stack dir_path;
    uint8_t path_buf[10];
    stack_init(&dir_path, path_buf, 9);

    enter("kernel in shell");

    char prompt[64];

    while (1)
    {
        puts("ax:");
        // 경로 출력하고
        // kpath();

        // 입력 받고
        gets(prompt, 63);

        // 입력을 정규화 및 적용
        ksplit(prompt);
    }
}