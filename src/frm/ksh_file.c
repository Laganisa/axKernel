#include "frm/ksh.h"
#include "ksh_internal.h"
#include "manage/_wm.h"

extern uint8_t global_dsk_mut;

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
uint8_t file_split(char **args, fcb_e *current_dir, uint8_t depth)
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
            log("1");
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

        wputs("file name : ");
        wputs((const char *)now_fcb->alias);
        wputs("\n");
        wputs("file len(B): ");
        fput_uint((uint64_t)now_fcb->lens);
        wputs("\n");
        wputs("file id: ");
        fput_uint((uint64_t)now_fcb->fid);
        wputs("\n");
        wputs("file auth : ");
        fput_uint((uint64_t)now_fcb->auth);
        wputs("\n");

        wputs("YYYY : ");
        fput_uint((uint64_t)now_fcb->YYYY);
        wputs("  MM : ");
        fput_uint((uint64_t)now_fcb->MM);
        wputs("  DD : ");
        fput_uint((uint64_t)now_fcb->DD);
        wputs("\n");
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

        wputs("dir name : ");
        wputs((const char *)now_dir_fcb->alias);
        wputs("\n");

        wputs("dir id: ");
        fput_uint((uint64_t)now_dir_fcb->fid);
        wputs("\n");
        wputs("dir auth : ");
        fput_uint((uint64_t)now_dir_fcb->auth);
        wputs("\n");

        if (now_dir_fcb->dir_num == 0)
        {
            kshputs_error("file not found\n");
        }
        else
        {
            wputs("files\n");
            wputs("----------\n");
            for (int i = 0; i < now_dir_fcb->dir_num; i++)
            {
                uint32_t file_pos = now_dir_fcb->files[i];
                fcb_t *now_file = &(fm_record->FMv3_mem[file_pos]);
                wputs("name : ");
                wputs((const char *)now_file->alias);
                wputs("  auth : ");
                fput_uint((uint64_t)now_file->auth);
                wputs("\n");
            }
            wputs("----------\n");
        }

        if (now_dir_fcb->dir_dnum == 0)
        {
            kshputs_error("dir not found\n");
        }
        else
        {
            wputs("\ndirs\n");
            wputs("----------\n");
            for (int i = 0; i < now_dir_fcb->dir_dnum; i++)
            {
                uint32_t dir_pos = now_dir_fcb->dirs[i];
                fcb_e *now_dir = &(fm_record->FMv3_dir_mem[dir_pos]);
                wputs("dir name : ");
                wputs((const char *)now_dir->alias);
                wputs("  auth : ");
                fput_uint((uint64_t)now_dir->auth);
                wputs("\n");
            }
            wputs("----------\n");
        }

        wputs("YYYY : ");
        fput_uint((uint64_t)now_dir_fcb->YYYY);
        wputs("  MM : ");
        fput_uint((uint64_t)now_dir_fcb->MM);
        wputs("  DD : ");
        fput_uint((uint64_t)now_dir_fcb->DD);
        wputs("\n");
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
            dsync
            dumut
        */
    // 디스크 동기화 명령
    case 0x838EBCC27262D6C4:

        if (args[1] != NULL)
        {
            kshputs_error("Syntax : file dsync\n");
            ret = ARGS_ERR;
            break;
        }

        if (!global_dsk_mut)
        {
            kshputs_error("Disk is not mounted\n");
            ret = COMM_ERR;
            break;
        }

        if (!fm_dsk_sync())
        {
            kshputs_error("Disk synchronization failed\n");
            ret = COMM_ERR;
            break;
        }

        wputs("Disk synchronized\n");
        break;

    // 디스크 언마운트 명령
    case 0xAECB9DF300CC8574:

        if (args[1] != NULL)
        {
            kshputs_error("Syntax : file dumut\n");
            ret = ARGS_ERR;
            break;
        }

        if (!global_dsk_mut)
        {
            kshputs_error("Disk is not mounted\n");
            ret = COMM_ERR;
            break;
        }

        if (!fm_dsk_unmount())
        {
            kshputs_error("Disk unmount failed\n");
            ret = COMM_ERR;
            break;
        }

        global_dsk_mut = FALSE;
        wputs("Disk unmounted\n");
        break;

    default:
        kshputs_error("Syntax : file {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
}

#pragma endregion

