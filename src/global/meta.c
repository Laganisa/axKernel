#include "global/_meta.h"
#include "manage/_mm.h"
#include "global/_debug.h"
#include "tools/_asm.h"
#include "manage/temp_mm.h"

#define PROC_STACK_RESERVE (16 * 1024ULL)
#define TEMP_PROC_PAGE_COUNT ((INITIAL_PROC_SIZE << 10) / TEMP_MM_PAGE_SIZE)
#define ELF_PF_EXEC 0x1
#define ELF_PF_WRITE 0x2
#define ELF_PF_READ 0x4

extern pcb_t *current_proc;

/*
    축약 함수
    ! 나중에 바꾸기
    만약 모드값이 1이라면 loader 함수를 호출하기
*/
pcb_t *proc_turn(
    FMv3_record *reco,
    int8_t *name,
    void *entry_point,
    uint8_t mod)
{
    /*
    dump("name", (uint64_t)name);
    dump("entry_point", (uint64_t)entry_point);
    dump("mod", (uint64_t)mod);
    */

    // 헤더 설정
    fm_exec_hdr_t new_hdr;
    new_hdr.magic = FM_EXEC_MAGIC;
    new_hdr.mode = (mod == 0) ? FM_EXEC_MODE_DIRECT : FM_EXEC_MODE_IMAGE;
    new_hdr.entry = (mod == 0) ? (uint64_t)entry_point : 0;
    new_hdr.image_size = 0;

    // IMAGE 모드일 때 바이너리 파일 기록
    if (mod == 1)
    {
        // ! 이거 바꾸기

        extern uint8_t _task_shell_start[];
        extern uint8_t _task_shell_size[];

        extern uint8_t _task_bridge_start[];
        extern uint8_t _task_bridge_size[];

        extern uint8_t _task_compiler_start[];
        extern uint64_t _task_compiler_size[];

        if (entry_point == (void *)_task_shell_start)
        {

            uint64_t shell_size = *((uint64_t *)_task_shell_size);

            // ELF 이미지와 FM 헤더를 함께 저장
            uint32_t total_size = (uint32_t)(sizeof(fm_exec_hdr_t) + shell_size);
            fcb_t *fil = fm_create(reco, name, total_size, 0);

            new_hdr.image_size = shell_size;
            fm_write(reco, fil, &new_hdr, sizeof(fm_exec_hdr_t), 0);

            fm_write(reco, fil, _task_shell_start, (uint32_t)shell_size, sizeof(fm_exec_hdr_t));

            return mata_exec_file(reco, pm_object, name, 0);
        }
        if (entry_point == (void *)_task_bridge_start)
        {
            uint64_t bridge_size = *((uint64_t *)_task_bridge_size);

            // ELF 이미지와 FM 헤더를 함께 저장
            uint32_t total_size = (uint32_t)(sizeof(fm_exec_hdr_t) + bridge_size);
            dump("total size", total_size);
            fcb_t *fil = fm_create(reco, name, total_size, 0);

            new_hdr.image_size = bridge_size;
            fm_write(reco, fil, &new_hdr, sizeof(fm_exec_hdr_t), 0);

            fm_write(reco, fil, _task_bridge_start, (uint32_t)bridge_size, sizeof(fm_exec_hdr_t));

            return mata_exec_file(reco, pm_object, name, 0);
        }
        if (entry_point == (void *)_task_compiler_start)
        {
            uint64_t compiler_size = *((uint64_t *)_task_compiler_size);

            // ELF 이미지와 FM 헤더를 함께 저장
            uint32_t total_size = (uint32_t)(sizeof(fm_exec_hdr_t) + compiler_size);
            dump("total size", total_size);
            fcb_t *fil = fm_create(reco, name, total_size, 0);

            new_hdr.image_size = compiler_size;
            fm_write(reco, fil, &new_hdr, sizeof(fm_exec_hdr_t), 0);

            fm_write(reco, fil, _task_compiler_start, (uint32_t)compiler_size, sizeof(fm_exec_hdr_t));

            return mata_exec_file(reco, pm_object, name, 0);
        }
    }
    // 커널 실행시 내부적으로 만든 파일이 실행되는 거
    else
    {
    }

    // 기본 동작
    fcb_t *fil = fm_create(reco, name, 1024, 0);
    fm_write(reco, fil, &new_hdr, sizeof(fm_exec_hdr_t), 0);
    return mata_exec_file(reco, pm_object, name, 0);
}

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

static pcb_t *elf_load_image(
    pcb_t *proc,
    uint8_t *image,
    uint32_t image_size)
{
    /*
    dump("proc", (uint64_t)proc);
    dump("image", (uint64_t)image);
    dump("image_size", (uint64_t)image_size);
    */

    elf_ehdr_t *ehdr = (elf_ehdr_t *)image;

    if (!elf_valid_header(ehdr, image_size))
    {
        return 0;
    }

    uint64_t min_vaddr = (uint64_t)-1;
    uint64_t max_vaddr = 0;
    uint64_t max_align = 1;

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
        if (phdr->p_vaddr + phdr->p_memsz > max_vaddr)
        {
            max_vaddr = phdr->p_vaddr + phdr->p_memsz;
        }
        if (phdr->p_align > max_align)
        {
            max_align = phdr->p_align;
        }
    }

    if (min_vaddr == (uint64_t)-1)
    {
        return 0;
    }

    if ((max_vaddr - min_vaddr) > (INITIAL_PROC_SIZE << 10))
    {
        return 0;
    }

    uint64_t real_addr = mm_find(&mm_stack, proc->mm_addr, 0);

    uint64_t load_base = real_addr;
    if (max_align > 1)
    {
        uint64_t align_mask = max_align - 1;
        load_base = (load_base + align_mask) & ~align_mask;
    }

    uint64_t process_size = INITIAL_PROC_SIZE << 10;
    uint64_t heap_limit = real_addr + process_size - PROC_STACK_RESERVE;
    uint64_t image_end = load_base + (max_vaddr - min_vaddr);
    uint64_t heap_start =
        (image_end + TEMP_MM_PAGE_SIZE - 1) & ~(TEMP_MM_PAGE_SIZE - 1);
    uint8_t page_flags[TEMP_PROC_PAGE_COUNT];
    uint8_t segment_flags[TEMP_PROC_PAGE_COUNT];
    uint8_t segment_seen[TEMP_PROC_PAGE_COUNT];

    for (uint16_t i = 0; i < TEMP_PROC_PAGE_COUNT; i++)
    {
        page_flags[i] = TEMP_MM_PAGE_READ | TEMP_MM_PAGE_WRITE;
        segment_flags[i] = 0;
        segment_seen[i] = FALSE;
    }

    if (heap_start > heap_limit)
    {
        return 0;
    }

    for (uint16_t i = 0; i < ehdr->e_phnum; i++)
    {
        elf_phdr_t *phdr = (elf_phdr_t *)(image + ehdr->e_phoff + (i * sizeof(elf_phdr_t)));
        if (phdr->p_type != ELF_PT_LOAD)
        {
            continue;
        }

        uint64_t seg_offset = phdr->p_vaddr - min_vaddr;
        uint64_t segment_start = load_base - real_addr + seg_offset;
        uint64_t segment_limit = process_size - PROC_STACK_RESERVE;
        if (segment_start > segment_limit ||
            phdr->p_memsz > segment_limit - segment_start)
        {
            return 0;
        }

        if (phdr->p_memsz == 0)
        {
            continue;
        }

        uint64_t segment_end = segment_start + phdr->p_memsz;
        uint64_t first_page = segment_start / TEMP_MM_PAGE_SIZE;
        uint64_t end_page =
            segment_end / TEMP_MM_PAGE_SIZE +
            (segment_end % TEMP_MM_PAGE_SIZE != 0);
        uint8_t flags = 0;
        if (phdr->p_flags & ELF_PF_READ)
        {
            flags |= TEMP_MM_PAGE_READ;
        }
        if (phdr->p_flags & ELF_PF_WRITE)
        {
            flags |= TEMP_MM_PAGE_WRITE;
        }
        if (phdr->p_flags & ELF_PF_EXEC)
        {
            flags |= TEMP_MM_PAGE_EXEC;
        }
        if (end_page > TEMP_PROC_PAGE_COUNT)
        {
            return 0;
        }
        for (uint64_t page = first_page; page < end_page; page++)
        {
            if (segment_seen[page])
            {
                segment_flags[page] |= flags;
            }
            else
            {
                segment_flags[page] = flags;
                segment_seen[page] = TRUE;
            }
            page_flags[page] = segment_flags[page];
        }

        uint8_t *dest = (uint8_t *)(load_base + seg_offset);
        memcpy(dest, image + phdr->p_offset, (uint32_t)phdr->p_filesz);

        for (uint64_t j = phdr->p_filesz; j < phdr->p_memsz; j++)
        {
            dest[j] = 0;
        }
    }

    if (ehdr->e_entry < min_vaddr)
    {
        return 0;
    }

    uint64_t entry_offset =
        load_base - real_addr + (ehdr->e_entry - min_vaddr);

    if (entry_offset >= process_size ||
        !(page_flags[entry_offset / TEMP_MM_PAGE_SIZE] & TEMP_MM_PAGE_EXEC))
    {
        return 0;
    }

    if (!temp_mmu_prepare_user(
            proc,
            proc->page_i,
            real_addr,
            TEMP_MM_USER_BASE,
            process_size,
            page_flags))
    {

        return 0;
    }

    proc->regs.elr_el1 = TEMP_MM_USER_BASE + entry_offset;
    proc->regs.sp = TEMP_MM_USER_BASE + process_size;
    proc->heap_start = TEMP_MM_USER_BASE + (heap_start - real_addr);
    proc->heap_break = proc->heap_start;
    proc->heap_limit =
        TEMP_MM_USER_BASE + process_size - PROC_STACK_RESERVE;

    return proc;
}

// 파일 실행
pcb_t *mata_exec_file(
    FMv3_record *reco,
    PMv1_object *obj,
    int8_t path[27],
    uint8_t parid)
{

    fcb_t *file = fm_find(reco, path);

    fm_exec_hdr_t *hdr;

    if (file == 0)
    {
        log("file zero error");
        return NULL;
    }

    if (file->lens < sizeof(fm_exec_hdr_t) ||
        file->lens > MAX_FILE_SIZE)
    {
        dump("invalid executable length", file->lens);
        return NULL;
    }

    hdr = (fm_exec_hdr_t *)fm_data_addr(reco, file);
    if (fm_dsk_ready() &&
        fm_read(reco, file, hdr, file->lens, 0) != file->lens)
    {
        log("executable read error");
        return NULL;
    }

    if (hdr->magic != FM_EXEC_MAGIC)
    {
        dump("magic", hdr->magic);
        log("magic error");
        return NULL;
    }

    if (hdr->mode == FM_EXEC_MODE_DIRECT)
    {
        return pm_create(obj, hdr->entry, parid);
    }

    if (hdr->mode == FM_EXEC_MODE_IMAGE)
    {

        pcb_t *proc = pm_create(obj, 0, parid);

        if (proc == 0)
        {
            return NULL;
        }

        if (elf_load_image(proc, ((uint8_t *)hdr) + sizeof(fm_exec_hdr_t), (uint32_t)hdr->image_size) == 0)
        {

            return NULL;
        }

        return proc;
    }
    return NULL;
}

pcb_t *schedule_proc(pcb_t *proc)
{

    // 다음 프로세스를 큐에서 꺼냄
    pcb_t *next = pm_run(pm_object);

    // ! 수정하기
    // 다른 프로세스를 꺼내는 상황
    if (next->state == (uint8_t)PROC_SIGNAL)
    {
        pm_awake(pm_object, 0, proc); // 현재 proc를 넣고
        mm_free(&mm_stack, &mm_substack, proc->mm_addr);
        current_proc = pm_run(pm_object); // 다른걸 꺼내자
    }
    else
    {
        pm_awake(pm_object, 0, proc);
        current_proc = next;
    }

    return current_proc;
}