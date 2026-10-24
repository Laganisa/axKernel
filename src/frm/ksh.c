#include "frm/ksh.h"
#include "frm/kedt.h"
#include "global/_io.h"

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

static stack dir_path;
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

static uint16_t kshputs(const char *s)
{
    return io_puts_color(s, 0xFFFFFFFF);
}

static uint16_t kshputs_error(const char *s)
{
    return io_puts_color(s, 0xFFFF0000);
}

static void fput_uint(uint64_t n)
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
    io_puts_color(&buf[i + 1], 0xFFFFFFFF);
    // kshputs(&buf[i + 1]);
}

// 현재 작업 경로를 나타내는 로직
// 스택에 저장된 경로 요소를 루트부터 출력
static void kpath()
{
    kshputs("ax:");
    kshputs("/");

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
            kshputs("/");
        }
        kshputs((const char *)fm_record->FMv3_dir_mem[dir_id].alias);
    }
    kshputs("> ");
}

#pragma endregion

// 메모리
static uint8_t mem_split(char **args)
{
    uint8_t ret;
    uint64_t hash_check = fnv1a_hash_64(args[0]);
    uint64_t targs[6] = {0};

    for (int i = 0; i < 6; i++)
    {
        targs[i] = str_to_uint(args[i]);
    }

    switch (hash_check)
    {

    default:
        kshputs_error("Syntax : mem {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
}

// 프로세스
static uint8_t proc_split(char **args)
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
    // 프로세스 생성
    case 0xF5A2F5190CAEA09C:
        if (prefix_targs[3] != 0)
        {
            kshputs_error("Syntax : file crt {entry}\n");
            ret = ARGS_ERR;
            break;
        }
        pm_create(pm_object, 1, 1);
        break;

    // 프로세스 삭제
    case 0xCA9A2418F461F5CA:

        break;

    // 프로세스 정보
    case 0xF5D6A8C57AB94F05:

        break;
        // 프로세스 상태(생명주기) 변경
    case 0xF5FAF8190CF91B8B:

        break;

    default:
        kshputs_error("Syntax : proc {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
}

#pragma region 파일

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

// 현재 디렉토리와 깊이를 기준으로 파일 명령을 처리
static uint8_t file_split(char **args, fcb_e *current_dir, uint8_t depth)
{
    uint8_t ret = COMM_OK;

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
    fcb_e *now_dir_fcb = NULL;

    switch (hash_check)
    {
    // 파일 생성 명령
    case 0xF5A2F5190CAEA09C:

        // 예외 처리
        // 생성파일의 크기가 너무 작거나 클때
        if (prefix_targs[4] != 0)
        {
            kshputs_error("Syntax : file crt {name/path} {size(B)} {auth}\n");
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

        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : file del {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        fm_delete(fm_record, args[1]);
        ret = COMM_OK;
        break;

    // 파일 정보
    case 0xf5d6a8c57ab94f05:

        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : file info {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_find(fm_record, args[1]);

        if (now_fcb == NULL)
        {
            kshputs_error("file not found\n");
            ret = COMM_ERR;
            break;
        }

        kshputs("file name : ");
        kshputs((const char *)now_fcb->alias);
        kshputs("\n");
        kshputs("file len(KB): ");
        fput_uint((uint64_t)now_fcb->lens);
        kshputs("\n");
        kshputs("file id: ");
        fput_uint((uint64_t)now_fcb->fid);
        kshputs("\n");
        kshputs("file auth : ");
        fput_uint((uint64_t)now_fcb->auth);
        kshputs("\n");

        kshputs("YYYY : ");
        fput_uint((uint64_t)now_fcb->YYYY);
        kshputs("  MM : ");
        fput_uint((uint64_t)now_fcb->MM);
        kshputs("  DD : ");
        fput_uint((uint64_t)now_fcb->DD);
        kshputs("\n");
        break;

    // 파일의 디렉토리간 이동
    case 0x08A95007B550372A:

        if (prefix_targs[5] != 0)
        {
            kshputs_error("Syntax : file mv {src dir name/path} {name/path} {dst dir name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_find(fm_record, args[2]);
        now_dir_fcb = fm_dir_find(fm_record, args[1]);
        fm_dir_out(fm_record, now_dir_fcb, now_fcb->fid);
        now_dir_fcb = fm_dir_find(fm_record, args[3]);
        fm_dir_in(fm_record, now_dir_fcb, now_fcb->fid);
        break;
    // 파일 권한 수정
    case 0xF5FAF8190CF91B8B:

        if ((prefix_targs[2] != 0) || (targs[2] > 512))
        {
            kshputs_error("Syntax : file chg {name/path} {chg auth}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_find(fm_record, args[1]);

        now_fcb->auth = targs[2];

        break;

    // 디렉토리 생성
    case 0x73430E67262B7B56:

        if (prefix_targs[4] != 0)
        {
            kshputs_error("Syntax : file dcrt {name/path} {size(B)} {auth}\n");
            ret = ARGS_ERR;
            break;
        }

        if (current_dir == NULL || depth >= 2)
        {
            kshputs_error("Directory depth limit reached\n");
            ret = COMM_ERR;
            break;
        }

        now_dir_fcb = fm_dir_create(
            fm_record,
            args[1],
            current_dir->fid,
            (uint16_t)(depth + 1),
            (uint16_t)targs[2]);

        if (now_dir_fcb == NULL)
        {
            ret = COMM_ERR;
        }
        else
        {
            ret = COMM_OK;
        }
        break;

    // 디렉토리 삭제
    case 0xB0DD41674955CBF4:
        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : file dir_del {name/path}\n");
            ret = ARGS_ERR;
            break;
        }
        fm_dir_delete(fm_record, args[1]);
        ret = COMM_OK;
        break;

        // 디렉토리 정보
    case 0x454A0E9939328967:

        if (prefix_targs[2] != 0)
        {
            kshputs_error("Syntax : file dir_info {name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        now_dir_fcb = fm_dir_find(fm_record, args[1]);

        if (now_dir_fcb == NULL)
        {
            ret = COMM_ERR;
            break;
        }

        kshputs("dir name : ");
        kshputs((const char *)now_dir_fcb->alias);
        kshputs("\n");

        kshputs("dir id: ");
        fput_uint((uint64_t)now_dir_fcb->fid);
        kshputs("\n");
        kshputs("dir auth : ");
        fput_uint((uint64_t)now_dir_fcb->auth);
        kshputs("\n");

        if (now_dir_fcb->dir_num == 0)
        {
            kshputs_error("file not found\n");
        }
        else
        {
            kshputs("files\n");
            kshputs("----------\n");
            for (int i = 0; i < now_dir_fcb->dir_num; i++)
            {
                uint32_t file_pos = now_dir_fcb->files[i];
                fcb_t *now_file = &(fm_record->FMv3_mem[file_pos]);
                kshputs("name : ");
                kshputs((const char *)now_file->alias);
                kshputs("  auth : ");
                fput_uint((uint64_t)now_file->auth);
                kshputs("\n");
            }
            kshputs("----------\n");
        }

        if (now_dir_fcb->dir_dnum == 0)
        {
            kshputs_error("dir not found\n");
        }
        else
        {
            kshputs("\ndirs\n");
            kshputs("----------\n");
            for (int i = 0; i < now_dir_fcb->dir_dnum; i++)
            {
                uint32_t dir_pos = now_dir_fcb->dirs[i];
                fcb_e *now_dir = &(fm_record->FMv3_dir_mem[dir_pos]);
                kshputs("dir name : ");
                kshputs((const char *)now_dir->alias);
                kshputs("  auth : ");
                fput_uint((uint64_t)now_dir->auth);
                kshputs("\n");
            }
            kshputs("----------\n");
        }

        kshputs("YYYY : ");
        fput_uint((uint64_t)now_dir_fcb->YYYY);
        kshputs("  MM : ");
        fput_uint((uint64_t)now_dir_fcb->MM);
        kshputs("  DD : ");
        fput_uint((uint64_t)now_dir_fcb->DD);
        kshputs("\n");
        break;

    // 디렉토리의 디렉토리간 이동
    case 0xCAB53A18F478E2E4:
        if (prefix_targs[5] != 0)
        {
            kshputs_error("Syntax : file dmv {name/path} {dst dir name/path}\n");
            ret = ARGS_ERR;
            break;
        }

        now_dir_fcb = fm_dir_find(fm_record, args[1]);
        fcb_e *new_dir_fcb = fm_dir_find(fm_record, args[2]);

        now_dir_fcb->p_fid = new_dir_fcb->fid;
        new_dir_fcb->dirs[new_dir_fcb->dir_dnum] = now_dir_fcb->fid;
        new_dir_fcb->dir_dnum++;

        break;

    // 디렉토리 권한 수정
    case 0x738E0D67266B651D:
        if ((prefix_targs[3] != 0) || targs[3] > 512)
        {
            kshputs_error("Syntax : file dchg {name/path} {auth}\n");
        }

        now_dir_fcb = fm_dir_find(fm_record, args[1]);
        now_dir_fcb->auth = targs[2];
        break;

    // 스코프 변화
    // 즉 디렉토리 이동
    case 0x822687195CCFB219:
        if (args[1] == NULL || args[3] != NULL)
        {
            kshputs_error("Syntax : file scp {in {dir name/path}|out}\n");
            ret = ARGS_ERR;
            break;
        }

        if (current_dir == NULL)
        {
            kshputs_error("Current directory unavailable\n");
            ret = COMM_ERR;
            break;
        }

        switch (fnv1a_hash_64(args[1]))
        {
        // 하위 디렉토리로 이동
        case 0x08B73807B55C4BBE:
            if (args[2] == NULL)
            {
                kshputs_error("Syntax : file scp in {dir name/path}\n");
                ret = ARGS_ERR;
                break;
            }

            now_dir_fcb = fm_dir_find(fm_record, args[2]);
            if (now_dir_fcb == NULL ||
                now_dir_fcb->p_fid != current_dir->fid ||
                now_dir_fcb->depth != depth + 1 ||
                dir_path.full(&dir_path))
            {
                kshputs_error("Directory not found in current path\n");
                ret = COMM_ERR;
                break;
            }

            dir_path.push(&dir_path, (uint8_t)now_dir_fcb->fid);
            break;

        // 상위 디렉토리로 이동
        case 0x19F79B1921BBCFFF:
            if (args[2] != NULL)
            {
                kshputs_error("Syntax : file scp out\n");
                ret = ARGS_ERR;
                break;
            }

            if (dir_path.count <= 1)
            {
                kshputs_error("Already at root directory\n");
                ret = COMM_ERR;
                break;
            }

            dir_path.pop(&dir_path);
            break;

        default:
            kshputs_error("Syntax : file scp {in {dir name/path}|out}\n");
            ret = ARGS_ERR;
            break;
        }
        break;

        // 나중에 디스크 관리
        /*

        */

    default:
        kshputs_error("Syntax : file {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
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

        break;

    // 장치 분리
    case 0x73A50667267E3819:

        break;

    // 장치 정보
    case 0xF5D6A8C57AB94F05:

        break;

    // 장치 열기
    case 0xF84F97B4633670E9:

        break;

    // 장치 닫기
    case 0x9EC2699513C0F9C3:

        break;

    default:
        kshputs_error("Syntax : dev {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
}

#pragma endregion

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

        // 화면을 지우고 호출하기

        // 화면 지우기

        // 호출
        kedt(now_fcb);
        break;

    default:
        kshputs_error("Syntax : sys {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

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
        fcb_e *current_dir = kcurrent_dir();
        uint8_t depth = dir_path.count == 0 ? 0 : (uint8_t)(dir_path.count - 1);
        ret = file_split(command, current_dir, depth);
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
        gets(prompt, 63);

        // 입력을 정규화 및 적용
        ksplit(prompt);
    }
}