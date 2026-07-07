#include "meta.h"
#include "mm.h"
#include "debug.h"
#include "asm.h"

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
    task.entry = (mod == 0) ? (uint64_t)entry_point : 0;
    task.image_size = 0;

    // 2. IMAGE 모드일 때 바이너리 파일 기록
    if (mod == 1)
    {
        extern uint8_t _task_shell_start[];
        extern uint8_t _task_shell_size[];

        if (entry_point == (void *)_task_shell_start)
        {
            uint64_t shell_size = *((uint64_t *)_task_shell_size);

            // ELF 이미지와 FM 헤더를 함께 저장
            uint32_t total_size = (uint32_t)(sizeof(fm_exec_hdr_t) + shell_size);
            uint32_t alloc_size = (uint32_t)(((total_size + 4095) / 4096) * 4096);

            fm_create(reco, name, alloc_size, 0);
            task.image_size = shell_size;
            fm_write(reco, name, &task, sizeof(fm_exec_hdr_t), 0);
            fm_write(reco, name, _task_shell_start, (uint32_t)shell_size, sizeof(fm_exec_hdr_t));

            return mata_exec_file(reco, &pm_object, name, 0);
        }
    }

    // 기본 동작
    fm_create(reco, name, 1024, 0);
    fm_write(reco, name, &task, sizeof(fm_exec_hdr_t), 0);
    return mata_exec_file(reco, &pm_object, name, 0);
}

/*
    ELF loader helper functions
*/

static uint8_t elf_valid_header(elf_ehdr_t *ehdr, uint32_t image_size)
{
    if (image_size < sizeof(elf_ehdr_t))
    {
        return FALSE;
    }
    if (ehdr->e_ident[0] != ELF_MAGIC0 || ehdr->e_ident[1] != ELF_MAG1 || ehdr->e_ident[2] != ELF_MAG2 || ehdr->e_ident[3] != ELF_MAG3)
    {
        return FALSE;
    }
    if (ehdr->e_ident[4] != ELF_CLASS_64 || ehdr->e_ident[5] != ELF_DATA_LSB)
    {
        return FALSE;
    }
    if (ehdr->e_machine != ELF_MACHINE_AARCH64)
    {
        return FALSE;
    }
    if (ehdr->e_phentsize != sizeof(elf_phdr_t))
    {
        return FALSE;
    }
    if ((uint64_t)ehdr->e_phoff + (uint64_t)ehdr->e_phnum * sizeof(elf_phdr_t) > image_size)
    {
        return FALSE;
    }
    return TRUE;
}

static pcb_t *elf_load_image(pcb_t *proc, uint8_t *image, uint32_t image_size)
{
    enter("elf load image");

    elf_ehdr_t *ehdr = (elf_ehdr_t *)image;

    if (!elf_valid_header(ehdr, image_size))
    {
        return 0;
    }

    uint64_t min_vaddr = (uint64_t)-1;
    for (uint16_t i = 0; i < ehdr->e_phnum; i++)
    {
        elf_phdr_t *phdr = (elf_phdr_t *)(image + ehdr->e_phoff + (i * sizeof(elf_phdr_t)));
        if (phdr->p_type != ELF_PT_LOAD)
        {
            continue;
        }
        if (phdr->p_filesz > phdr->p_memsz)
        {
            return 0;
        }
        if (phdr->p_offset + phdr->p_filesz > image_size)
        {
            return 0;
        }
        if (phdr->p_vaddr < min_vaddr)
        {
            min_vaddr = phdr->p_vaddr;
        }
    }

    if (min_vaddr == (uint64_t)-1)
    {
        return 0;
    }

    uint64_t real_addr = mm_find(&mm_stack, proc->mm_addr, 0);

    for (uint16_t i = 0; i < ehdr->e_phnum; i++)
    {
        elf_phdr_t *phdr = (elf_phdr_t *)(image + ehdr->e_phoff + (i * sizeof(elf_phdr_t)));
        if (phdr->p_type != ELF_PT_LOAD)
        {
            continue;
        }

        uint64_t seg_offset = phdr->p_vaddr - min_vaddr;
        if (seg_offset + phdr->p_memsz > (INITIAL_PROC_SIZE << 10))
        {
            return 0;
        }

        uint8_t *dest = (uint8_t *)(real_addr + seg_offset);
        memcpy(dest, image + phdr->p_offset, (uint32_t)phdr->p_filesz);

        dump("p_offset", phdr->p_offset);
        dump("p_vaddr", phdr->p_vaddr);
        dump("p_filesz", phdr->p_filesz);

        dump("dest+0x840", *(uint64_t *)(dest + 0x840));
        dump("dest+0x848", *(uint64_t *)(dest + 0x848));
        dump("real_addr", real_addr);
        dump("min_vaddr", min_vaddr);
        dump("entry", ehdr->e_entry);

        for (uint64_t j = phdr->p_filesz; j < phdr->p_memsz; j++)
        {
            dest[j] = 0;
        }
    }

    if (ehdr->e_entry < min_vaddr)
    {
        return 0;
    }

    proc->elr_el1 = real_addr + (ehdr->e_entry - min_vaddr);
    return proc;
}

// 파일 실행
pcb_t *mata_exec_file(FMv2_record *reco, PMv1_object *obj, int8_t path[27], uint8_t parid)
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

        if (proc == 0)
        {
            return 0;
        }

        if (elf_load_image(proc, ((uint8_t *)hdr) + sizeof(fm_exec_hdr_t), (uint32_t)hdr->image_size) == 0)
        {
            return 0;
        }

        return proc;
    }

    return 0;
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
