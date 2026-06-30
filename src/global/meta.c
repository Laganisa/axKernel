#include "meta.h"
#include "mm.h"
#include "debug.h"

extern pcb_t *current_proc;

/*
    축약 함수
    ! 나중에 바꾸기
*/
pcb_t *proc_turn(FMv2_record *reco, int8_t *name, void *entry_point, uint8_t mod)
{
    fm_exec_hdr_t task;
    task.magic = FM_EXEC_MAGIC;

    if (mod == 0)
    {
        task.mode = FM_EXEC_MODE_DIRECT;
    }
    else if (mod == 1)
    {
        task.mode = FM_EXEC_MODE_IMAGE;
    }

    task.entry = (uint64_t)entry_point;
    task.image_size = 0;

    if (mod == 1)
    {
        // IMAGE 모드: entry_point가 메모리상의 바이너리일 경우 그 내용을 파일에 기록
        extern uint8_t _shell_binary_start[];
        extern uint8_t _shell_binary_size[];

        if (entry_point == (void *)_shell_binary_start)
        {
            uint64_t shell_size = *((uint64_t *)_shell_binary_size);
            uint64_t shell_header_size = sizeof(fm_exec_hdr_t);
            uint64_t shell_payload_size = 0;
            uint8_t *shell_payload = _shell_binary_start;

            if (shell_size > shell_header_size)
            {

                shell_payload_size = shell_size - shell_header_size;
                shell_payload = _shell_binary_start + shell_header_size;
            }
            else
            {

                shell_payload_size = shell_size;
            }

            task.entry = 0; // payload 시작을 엔트리로 사용
            task.image_size = shell_payload_size;

            uint64_t total = sizeof(fm_exec_hdr_t) + shell_payload_size;
            uint32_t alloc_size = (uint32_t)(((total + 1023) / 1024) * 1024);

            fcb_t *created = fm_create(reco, name, alloc_size, 0);
            uint32_t w1 = fm_write(reco, name, &task, sizeof(fm_exec_hdr_t), 0);
            uint32_t w2 = fm_write(reco, name, shell_payload, (uint32_t)shell_payload_size, sizeof(fm_exec_hdr_t));
            pcb_t *p = fm_exec_file(reco, &pm_object, name, 0);

            return p;
        }
    }

    // 기본 동작: DIRECT 모드 혹은 IMAGE 모드가지만 특별 데이터 없음
    fm_create(reco, name, 1024, 0);
    fm_write(reco, name, &task, sizeof(fm_exec_hdr_t), 0);

    return fm_exec_file(reco, &pm_object, name, 0);
}

pcb_t *schedule_proc(pcb_t *proc)
{

    pcb_t *next = pm_run(&pm_object);

    dump("next", next);

    if (next == (pcb_t *)PROC_SIGNAL)
    {
        pm_awake(&pm_object, 0, proc); // 현재 proc를 넣고
        mm_free(&mm_stack, &mm_substack, proc->mm_addr);
        current_proc = pm_run(&pm_object); // 다른걸 꺼내자
    }
    else
    {
        pm_awake(&pm_object, 0, proc);
        current_proc = next;
    }

    return next;
}