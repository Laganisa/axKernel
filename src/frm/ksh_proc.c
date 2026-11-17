#include "frm/ksh.h"
#include "ksh_internal.h"

uint8_t proc_split(char **args)
{
    kshputs_error("sorry yet devo\n");
    return 0;

    /*
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
    pcb_t *now_pcb = NULL;

    switch (hash_check)
    {
    // 프로세스 생성
    case 0xF5A2F5190CAEA09C:
        if (prefix_targs[3] != 0)
        {
            kshputs_error("Syntax : proc crt {entry} {proc name}\n");
            ret = ARGS_ERR;
            break;
        }
        now_fcb = fm_find(fm_record, args[2]);

        if (now_fcb == NULL)
        {
            kshputs_error("file not found\n");
            ret = COMM_ERR;
            break;
        }

        // todo 파일의 실행 권한을 확인한다음 실행하기
        if (now_fcb->auth == 1)
        {
            kshputs_error("Syntax : proc crt {entry} {proc name}\n");
            ret = ARGS_ERR;
            break;
        }

        // todo 임시로 모드값 2를 줌
        now_pcb = proc_turn(fm_record, args[3], args[2], 2);

        break;

    // 프로세스 삭제
    case 0xCA9A2418F461F5CA:
        if (prefix_targs[3] != 0)
        {
            kshputs_error("Syntax : proc del {entry}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_find(fm_record, args[2]);

        if (now_fcb == NULL)
        {
            kshputs_error("file not found\n");
            ret = COMM_ERR;
            break;
        }

        // todo 파일의 권한을 확인
        if (now_fcb->auth == 1)
        {
            kshputs_error("Syntax : proc crt {entry} {proc name}\n");
            ret = ARGS_ERR;
            break;
        }

        // todo 프로세스 삭제 로직

        break;

    // 프로세스 정보
    case 0xF5D6A8C57AB94F05:
        if (prefix_targs[3] != 0)
        {
            kshputs_error("Syntax : proc info {entry}\n");
            ret = ARGS_ERR;
            break;
        }

        now_fcb = fm_find(fm_record, args[2]);

        if (now_fcb == NULL)
        {
            kshputs_error("file not found\n");
            ret = COMM_ERR;
            break;
        }

        // todo 보기 권한이 있는지 확인
        if (now_fcb->auth == 1)
        {
            kshputs_error("Syntax : proc crt {entry} {proc name}\n");
            ret = ARGS_ERR;
            break;
        }

        kshputs("proc id : ");
        fput_uint(now_pcb->id);
        kshputs("  proc pid : ");
        fput_uint(now_pcb->p_id);
        kshputs("  proc state : ");

        // 스케쥴링 정책이 완료되면

        if (now_pcb->state)
        {
        }
        else if (now_pcb->state)
        {
        }


        kshputs("\nproc control : ");

        for (int i = 0; i < MAX_CONTROL_NUM; i++)
        {
            ctrl_t now_ctrl = now_pcb->control[i];

            if (now_ctrl.is_ctrl_alloc)
            {

                if (now_ctrl.is_file)
                {
                    fcb_t *new_fcb = now_ctrl.use_file;
                    kshputs((const char *)new_fcb->alias);
                }
                else
                {
                    dcb_t *now_dcb = now_ctrl.use_dev;
                    kshputs((const char *)now_dcb->name);
                }
                kshputs("\n");
            }
            else
            {
                kshputs("not control alloc\n");
            }
        }

        break;

        // 프로세스 상태(생명주기) 변경
    case 0xF5FAF8190CF91B8B:
        if (prefix_targs[3] != 0)
        {
            kshputs_error("Syntax : proc chg {mod}\n");
            ret = ARGS_ERR;
            break;
        }

        // todo  권한이 있는지 확인 실행 권한이 있어야 함
        now_fcb = fm_find(fm_record, args[2]);

        if (now_fcb == NULL)
        {
            kshputs_error("file not found\n");
            ret = COMM_ERR;
            break;
        }

        if (now_fcb->auth == 1)
        {
            kshputs_error("Syntax : proc crt {entry} {proc name}\n");
            ret = ARGS_ERR;
            break;
        }

        // todo 큐에서 위치 변경

        break;

    default:
        kshputs_error("Syntax : proc {command} {args}\n");
        ret = COMM_ERR;
        break;
    }

    return ret;
    */
}

