#include "../include/fm.h"

#ifndef __META_H__
#define __META_H__

// ELF 헤더 구조체
typedef struct elf_ehdr
{
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf_ehdr_t;

typedef struct elf_phdr
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf_phdr_t;

#define ELF_PT_LOAD 1
#define ELF_MAGIC0 0x7F
#define ELF_MAG1 'E'
#define ELF_MAG2 'L'
#define ELF_MAG3 'F'
#define ELF_CLASS_64 2
#define ELF_DATA_LSB 1
#define ELF_MACHINE_AARCH64 183

pcb_t *proc_turn(FMv2_record *reco, int8_t *name, void *entry_point, uint8_t mod);
pcb_t *schedule_proc(pcb_t *proc);
pcb_t *mata_exec_file(FMv2_record *reco, PMv1_object *obj, int8_t path[27], uint8_t parid);
#endif