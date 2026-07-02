#include "meta.h"
#include "mm.h"
#include "debug.h"

extern pcb_t *current_proc;

/*
    축약 함수
    ! 나중에 바꾸기
    만약 모드값이 1이라면 loader 함수를 호출하기
*/
pcb_t *proc_turn(FMv2_record *reco, int8_t *name, void *entry_point, uint8_t mod)
{
    // 1. 헤더 설정 (이제는 정보 전달용으로만 사용)
    fm_exec_hdr_t task;
    task.magic = FM_EXEC_MAGIC;
    task.mode = (mod == 0) ? FM_EXEC_MODE_DIRECT : FM_EXEC_MODE_IMAGE;
    task.entry = (uint64_t)entry_point;
    task.image_size = 0;

    // 2. IMAGE 모드일 때 바이너리 파일 기록
    if (mod == 1)
    {
        extern uint8_t _task_shell_start[];
        extern uint8_t _task_shell_size[];

        if (entry_point == (void *)_task_shell_start)
        {
            uint64_t shell_size = *((uint64_t *)_task_shell_size);

            // 헤더를 앞에 붙이지 않고, 전체 바이너리를 파일에 통째로 기록
            // 그래야 링커가 배치한 데이터 섹션들이 그대로 유지됨
            uint32_t alloc_size = (uint32_t)(((shell_size + 4095) / 4096) * 4096);

            fm_create(reco, name, alloc_size, 0);

            // task 헤더를 파일 앞에 쓰고 싶다면 쓰되,
            // 바이너리 데이터는 오프셋 0부터 시작하도록 유지해야 함
            // 만약 FM_exec_file이 오프셋을 처리한다면, 헤더를 건너뛰도록 하거나
            // 아예 헤더를 파일 외부에 두는 설계를 고려해 봐.
            fm_write(reco, name, _task_shell_start, (uint32_t)shell_size, 0);

            return fm_exec_file(reco, &pm_object, name, 0);
        }
    }

    // 기본 동작
    fm_create(reco, name, 1024, 0);
    fm_write(reco, name, &task, sizeof(fm_exec_hdr_t), 0);
    return fm_exec_file(reco, &pm_object, name, 0);
}

void loader(void)
{
    ;
}

pcb_t *schedule_proc(pcb_t *proc)
{

    // 다음 프로세스를 큐에서 꺼냄
    pcb_t *next = pm_run(&pm_object);

    // ! 수정하기
    // 다른 프로세스를 꺼내는 상황
    if (next->state == (uint8_t)PROC_SIGNAL)
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
