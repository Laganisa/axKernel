#include "frm/ksh.h"
#include "ksh_internal.h"
#include "frm/kedt.h"
#include "global/_io.h"
#include "global/_meta.h"
#include "manage/_wm.h"
#include "manage/_gm.h"

enum shell_incode
{
    PIPE_INCO = 1641,
    BACK_INCO = 1477,
    REDI_INCO = 1621,
    SCRI_INCO = 1649,
};

stack dir_path;
static uint8_t path_buf[3];

#pragma region shell 내부 함수

static fcb_e *kcurrent_dir(void)
{
    if (dir_path.empty(&dir_path))
    {
        return NULL;
    }

    uint8_t dir_id = dir_path.buffer[dir_path.head - 1];
    if (dir_id >= fm_record->all_dir_num)
    {
        return NULL;
    }

    return &(fm_record->FMv3_dir_mem[dir_id]);
}

uint16_t kshputs_error(const char *s)
{
    return wputs_color(s, 0xFFFF0000);
}

void fput_uint(uint64_t n)
{
    int8_t buf[21];
    int32_t i = 19;
    buf[20] = '\0';

    if (n == 0)
        buf[i--] = '0';

    while (n > 0 && i >= 0)
    {
        buf[i--] = (n % 10) + '0';
        n /= 10;
    }
    wputs((const char *)&buf[i + 1]);
}

// 현재 작업 경로를 나타내는 로직
// 스택에 저장된 경로 요소를 루트부터 출력
static void kpath()
{
    wputs("ax:");
    wputs("/");

    for (uint32_t i = 0; i < dir_path.count; i++)
    {
        uint8_t dir_id = dir_path.buffer[i];
        if (dir_id >= fm_record->all_dir_num)
        {
            kshputs_error("(invalid path)");
            return;
        }

        if (i != 0)
        {
            wputs("/");
        }
        wputs((const char *)fm_record->FMv3_dir_mem[dir_id].alias);
    }
    wputs("> ");
}

#pragma endregion

#pragma region 장치

/*
    장치

    con
    dcon
    info
    open
    close
*/
static uint8_t dev_split(char **args)
{
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    for (int i = 0; i < 6; i++)
    {
        targs[i] = str_to_uint(args[i]);
    }

    // 누적 OR 으로 나중에 인자 개수 판별
    uint8_t prefix_targs[6] = {0};

    for (int i = 5; i >= 0; i--)
    {
        prefix_targs[i] = targs[i] | (i < 5 ? prefix_targs[i + 1] : 0);
    }

    switch (hash_check)
    {
    // 장치 연결
    case 0xF604F1190D01642B:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : dev con {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        break;

    // 장치 분리
    case 0x73A50667267E3819:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : dev dcon {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        break;

    // 장치 정보
    case 0xF5D6A8C57AB94F05:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : dev info {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        break;

    // 장치 열기
    case 0xF84F97B4633670E9:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : dev opn {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        break;

    // 장치 닫기
    case 0x9EC2699513C0F9C3:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : dev cls {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        break;

    default:
        kshputs_error("Syntax : dev {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
}

#pragma region 시스템

// 시스템 처리
// 사실 잡다한 것들 용도
static uint8_t sys_split(char **args)
{
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    for (int i = 0; i < 6; i++)
    {
        targs[i] = str_to_uint(args[i]);
    }

    // 누적 OR 으로 나중에 인자 개수 판별
    uint8_t prefix_targs[6] = {0};

    for (int i = 5; i >= 0; i--)
    {
        prefix_targs[i] = targs[i] | (i < 5 ? prefix_targs[i + 1] : 0);
    }

    fcb_t *now_fcb = NULL;

    switch (hash_check)
    {
    // 에디터 연결
    case 0xC2CE0F18F0367E08:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : sys edt {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_find(fm_record, args[1]);

        if (now_fcb == NULL)
        {
            kshputs_error("unknow file\n");
            ret = UNKNOW_ERR;
            break;
        }

        if (now_fcb->lens == 0)
        {
            kshputs_error("file size err\n");
            ret = UNKNOW_ERR;
            break;
        }

        // 화면을 지우고 호출하기

        // 화면 지우기
        gm_rect(0, 0, 640, 360, 0xFF000000, 0, 0, 640, 360);
        // 호출
        kedt(now_fcb);

        // 정리하기
        gm_rect(0, 0, 640, 360, 0xFF000000, 0, 0, 640, 360);
        break;

    default:
        kshputs_error("Syntax : sys {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
}

#pragma endregion

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

    // 리턴 변수 받는 부분
    uint8_t ret = UNKNOW_ERR;

    // 메모리
    if (incode == 747 && (con & 0b1))
    {
        kshputs_error("sorry yet devo\n");
        // ret = mem_split(command);
    }
    // 프로세스
    else if (incode == 1673 &&
             ((con >> 1 & 0b1)))
    {
        // kshputs_error("sorry yet devo\n");
        ret = proc_split(command);
    }
    // 파일
    else if (incode == 1553 &&
             ((con >> 2) & 0b1))
    {
        fcb_e *current_dir = kcurrent_dir();
        uint8_t depth = dir_path.count == 0 ? 0 : (uint8_t)(dir_path.count - 1);
        ret = file_split(command, current_dir, depth);
    }
    // 장치
    else if (incode == 1673 &&
             ((con >> 3) & 0b1))
    {
        // kshputs_error("sorry yet devo\n");
        // ret = dev_split(command);
    }
    // 시스템
    else if (incode == 817)
    {
        // kshputs_error("sorry yet devo\n");
        ret = sys_split(command);
    }

    // ret에 맞는 처리
    switch (ret)
    {
    // 잘못된 범주 선택
    case COMM_OK:
        break;

    case UNKNOW_ERR:
        kshputs_error("Invalid category selected\n");
        break;

    // 문법 오류
    case GRMMER_ERR:
        kshputs_error("Grammar error\n");
        break;

    // 인자 전달 오류
    case ARGS_ERR:
        kshputs_error("Invalid arguments\n");
        break;

    // 명령 오류
    case COMM_ERR:
        kshputs_error("Command error\n");
        break;
    // 그 외 예상치 못한 오류
    default:
        kshputs_error("Unexpected error\n");
        break;
    }
}

// 기초적인 커널 내장 쉘
void kshell(void)
{
    enable_irq();

    // 경로 설정
    stack_init(&dir_path, path_buf, sizeof(path_buf));
    fcb_e *root_dir = fm_dir_find(fm_record, "root");

    if (root_dir == NULL)
    {
        kshputs_error("Shell root directory unavailable\n");
        return;
    }

    dir_path.push(&dir_path, (uint8_t)root_dir->fid);

    enter("kernel in shell");

    char prompt[64];

    while (1)
    {
        // 경로 출력하고
        kpath();

        // 입력 받고
        if (wget(prompt, sizeof(prompt)) < 0)
        {
            return;
        }

        // 입력을 정규화 및 적용
        ksplit(prompt);
    }
}