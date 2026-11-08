#include "manage/_fm.h"
#include "global/_io.h"
#include "manage/_mm.h"
#include "tools/_asm.h"
#include "tools/_fs.h"
#include "global/_debug.h"
#include "global/_alloc.h"
#include "tools/_hash.h"
#include "tools/_dsk_fs.h"
#include "tools/_virtio.h"

/*
    fm 로직에서 벗어난 팩 언팩 관련 파일
*/

typedef struct fm_dsk_pending
{
    uint32_t first_sector;
    uint32_t byte_length;
    uint8_t released;
    struct fm_dsk_pending *next;
} fm_dsk_pending;

static fm_dsk_pending *fm_dsk_pending_head;
static fm_dsk_pending *fm_dsk_pending_tail;
static uint8_t fm_dsk_active;
static FMv3_record *fm_dsk_record;
static uint8_t fm_dsk_buffer[FM_PACK_BUFFER_SIZE]
    __attribute__((aligned(16)));

static pack_fcb_t fm_fcb_t_pack(const fcb_t *now_fcb)
{
    pack_fcb_t packed;
    memcpy(packed.alias, now_fcb->alias, sizeof(packed.alias));
    packed.checksum = now_fcb->checksum;
    packed.depth = (uint8_t)now_fcb->depth;
    packed.auth = now_fcb->auth;
    packed.YYYMMDD =
        (uint16_t)(((uint16_t)now_fcb->YYYY << 9) |
                   ((uint16_t)now_fcb->MM << 5) |
                   (uint16_t)now_fcb->DD);
    packed.lens = now_fcb->lens;
    packed.dsk_id = now_fcb->dsk_id;
    return packed;
}

static pack_fcb_e fm_fcb_e_pack(const fcb_e *now_fcb)
{
    pack_fcb_e packed;
    memcpy(packed.alias, now_fcb->alias, sizeof(packed.alias));
    packed.dir_num = (uint8_t)now_fcb->dir_num;
    packed.dir_dnum = (uint8_t)now_fcb->dir_dnum;
    packed.auth = now_fcb->auth;
    packed.dsk_id = now_fcb->dsk_id;
    packed.checksum = (uint8_t)now_fcb->checksum;
    packed.YYYMMDD =
        (uint16_t)(((uint16_t)now_fcb->YYYY << 9) |
                   ((uint16_t)now_fcb->MM << 5) |
                   (uint16_t)now_fcb->DD);
    return packed;
}

static uint8_t fm_pack_append(
    fm_pack_context *context,
    const void *record,
    uint32_t record_size)
{
    if (context->offset > context->capacity ||
        record_size > context->capacity - context->offset)
    {
        return FALSE;
    }

    memcpy(context->buffer + context->offset, record, record_size);
    context->offset += record_size;
    return TRUE;
}

static uint8_t is_fm_chk_name(const int8_t *name)
{
    if (name == NULL || name[0] == '\0')
    {
        return FALSE;
    }

    for (uint32_t i = 1; i <= MAX_FILE_NAME; i++)
    {
        if (name[i] == '\0')
        {
            return TRUE;
        }
    }
    return FALSE;
}

static uint8_t fm_pack_dir(
    fm_pack_context *context,
    const fcb_e *directory,
    uint8_t is_root)
{
    FMv3_record *reco = context->reco;
    uint16_t dir_id;

    if (directory == NULL ||
        directory->dir_num > MAX_DIR_FILE_NUM ||
        directory->dir_dnum > MAX_DIR_FILE_NUM ||
        directory->depth > 2 ||
        !is_fm_chk_name(directory->alias))
    {
        return FALSE;
    }

    dir_id = directory->fid;
    if (dir_id == 0 || dir_id >= MAX_DIR_NUM ||
        dir_id >= reco->all_dir_num ||
        &reco->FMv3_dir_mem[dir_id] != directory ||
        context->seen_dirs[dir_id])
    {
        return FALSE;
    }
    context->seen_dirs[dir_id] = TRUE;

    if (is_root)
    {
        if (directory->depth != 0)
        {
            return FALSE;
        }
    }
    else
    {
        pack_fcb_e packed_directory = fm_fcb_e_pack(directory);
        if (!fm_pack_append(
                context,
                &packed_directory,
                sizeof(packed_directory)))
        {
            return FALSE;
        }
        context->dir_count++;
    }

    for (uint32_t i = 0; i < directory->dir_num; i++)
    {
        uint16_t file_id = directory->files[i];
        if (file_id == 0 || file_id >= reco->all_file_num ||
            file_id >= MAX_FILE_NUM || context->seen_files[file_id])
        {
            return FALSE;
        }

        fcb_t *file = &reco->FMv3_mem[file_id];
        if (!file->is_alloc || file->fid != file_id ||
            !is_fm_chk_name(file->alias))
        {
            return FALSE;
        }

        pack_fcb_t packed_file = fm_fcb_t_pack(file);
        if (!fm_pack_append(context, &packed_file, sizeof(packed_file)))
        {
            return FALSE;
        }
        context->seen_files[file_id] = TRUE;
        context->file_count++;
    }

    for (uint32_t i = 0; i < directory->dir_dnum; i++)
    {
        uint16_t child_id = directory->dirs[i];
        if (child_id == 0 || child_id >= reco->all_dir_num ||
            child_id >= MAX_DIR_NUM)
        {
            return FALSE;
        }

        fcb_e *child = &reco->FMv3_dir_mem[child_id];
        if (child->p_fid != dir_id ||
            child->depth != directory->depth + 1)
        {
            return FALSE;
        }
        if (!fm_pack_dir(context, child, FALSE))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static fcb_t *fm_fcb_t_unpack(
    FMv3_record *reco,
    const pack_fcb_t *packed,
    fcb_e *parent)
{
    if (!is_fm_chk_name(packed->alias) ||
        packed->lens > MAX_FILE_SIZE ||
        packed->auth > 0x3FF ||
        parent == NULL ||
        parent->dir_num >= MAX_DIR_FILE_NUM ||
        reco->all_file_num >= MAX_FILE_NUM)
    {
        return NULL;
    }

    uint16_t file_id = reco->all_file_num;
    if (bpt_insert(reco->root, (char *)packed->alias, file_id) == 0)
    {
        return NULL;
    }

    fcb_t *file = &reco->FMv3_mem[file_id];
    memset(file, 0, sizeof(*file));
    memcpy(file->alias, packed->alias, sizeof(file->alias));
    file->is_alloc = TRUE;
    file->fid = file_id;
    file->checksum = packed->checksum;
    file->depth = packed->depth;
    file->auth = packed->auth;
    file->YYYY = (packed->YYYMMDD >> 9) & 0x7F;
    file->MM = (packed->YYYMMDD >> 5) & 0x0F;
    file->DD = packed->YYYMMDD & 0x1F;
    file->lens = packed->lens;
    file->dsk_id = packed->dsk_id;

    reco->all_file_num++;
    if (!fm_dir_in(reco, parent, file_id))
    {
        return NULL;
    }
    return file;
}

static fcb_e *fm_fcb_e_unpack(
    FMv3_record *reco,
    const pack_fcb_e *packed,
    fcb_e *parent,
    uint16_t depth)
{
    if (!is_fm_chk_name(packed->alias) ||
        packed->dir_num > MAX_DIR_FILE_NUM ||
        packed->dir_dnum > MAX_DIR_FILE_NUM ||
        packed->auth > 0x3FF ||
        parent == NULL ||
        parent->dir_dnum >= MAX_DIR_FILE_NUM ||
        depth == 0 || depth > 2 ||
        reco->all_dir_num >= MAX_DIR_NUM)
    {
        return NULL;
    }

    fcb_e *directory = fm_dir_create(
        reco,
        (char *)packed->alias,
        parent->fid,
        depth,
        packed->auth);
    if (directory == NULL)
    {
        return NULL;
    }

    directory->dsk_id = packed->dsk_id;
    directory->checksum = packed->checksum;
    directory->YYYY = (packed->YYYMMDD >> 9) & 0x7F;
    directory->MM = (packed->YYYMMDD >> 5) & 0x0F;
    directory->DD = packed->YYYMMDD & 0x1F;
    return directory;
}

static uint8_t fm_unpack_read(
    fm_unpack_context *context,
    void *record,
    uint32_t record_size)
{
    if (context->offset > context->size ||
        record_size > context->size - context->offset)
    {
        return FALSE;
    }

    memcpy(record, context->buffer + context->offset, record_size);
    context->offset += record_size;
    return TRUE;
}

static uint8_t fm_unpack_chk_contents(
    fm_unpack_context *context,
    uint8_t file_count,
    uint8_t dir_count,
    uint16_t depth)
{
    if (file_count > MAX_DIR_FILE_NUM ||
        dir_count > MAX_DIR_FILE_NUM ||
        depth > 2)
    {
        return FALSE;
    }

    for (uint32_t i = 0; i < file_count; i++)
    {
        pack_fcb_t packed_file;
        if (!fm_unpack_read(context, &packed_file, sizeof(packed_file)) ||
            !is_fm_chk_name(packed_file.alias) ||
            packed_file.lens > MAX_FILE_SIZE ||
            packed_file.auth > 0x3FF ||
            packed_file.depth > 2)
        {
            return FALSE;
        }
        context->file_count++;
    }

    for (uint32_t i = 0; i < dir_count; i++)
    {
        pack_fcb_e packed_directory;
        if (!fm_unpack_read(
                context,
                &packed_directory,
                sizeof(packed_directory)) ||
            !is_fm_chk_name(packed_directory.alias) ||
            packed_directory.dir_num > MAX_DIR_FILE_NUM ||
            packed_directory.dir_dnum > MAX_DIR_FILE_NUM ||
            packed_directory.auth > 0x3FF ||
            depth >= 2 ||
            (packed_directory.alias[0] == 'r' &&
             packed_directory.alias[1] == 'o' &&
             packed_directory.alias[2] == 'o' &&
             packed_directory.alias[3] == 't' &&
             packed_directory.alias[4] == '\0'))
        {
            return FALSE;
        }

        context->dir_count++;
        if (!fm_unpack_chk_contents(
                context,
                packed_directory.dir_num,
                packed_directory.dir_dnum,
                depth + 1))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static uint8_t fm_unpack_dir_contents(
    fm_unpack_context *context,
    fcb_e *directory,
    uint8_t file_count,
    uint8_t dir_count,
    uint16_t depth)
{
    if (directory == NULL ||
        file_count > MAX_DIR_FILE_NUM ||
        dir_count > MAX_DIR_FILE_NUM ||
        depth > 2)
    {
        return FALSE;
    }

    for (uint32_t i = 0; i < file_count; i++)
    {
        pack_fcb_t packed_file;
        if (!fm_unpack_read(context, &packed_file, sizeof(packed_file)) ||
            packed_file.depth > 2 ||
            fm_fcb_t_unpack(context->reco, &packed_file, directory) == NULL)
        {
            return FALSE;
        }
        context->file_count++;
    }

    for (uint32_t i = 0; i < dir_count; i++)
    {
        pack_fcb_e packed_directory;
        if (!fm_unpack_read(
                context,
                &packed_directory,
                sizeof(packed_directory)))
        {
            return FALSE;
        }

        fcb_e *child = fm_fcb_e_unpack(
            context->reco,
            &packed_directory,
            directory,
            depth + 1);
        if (child == NULL)
        {
            return FALSE;
        }
        context->dir_count++;

        if (!fm_unpack_dir_contents(
                context,
                child,
                packed_directory.dir_num,
                packed_directory.dir_dnum,
                depth + 1))
        {
            return FALSE;
        }
    }

    return directory->dir_num == file_count &&
           directory->dir_dnum == dir_count;
}

uint32_t fm_pack(
    FMv3_record *reco,
    uint8_t *buffer,
    uint32_t capacity)
{
    if (reco == NULL || reco->root == NULL || reco->dir_root == NULL ||
        buffer == NULL ||
        capacity < sizeof(fm_pack_header))
    {
        return 0;
    }

    fcb_e *root = fm_dir_find(reco, "root");
    if (root == NULL)
    {
        return 0;
    }

    fm_pack_context context;
    memset(&context, 0, sizeof(context));
    context.reco = reco;
    context.buffer = buffer;
    context.capacity = capacity;
    context.offset = sizeof(fm_pack_header);

    if (!fm_pack_dir(&context, root, TRUE))
    {
        return 0;
    }

    fm_pack_header header;

    header.magic = FM_PACK_MAGIC;
    header.version = FM_PACK_VERSION;
    header.header_size = sizeof(header);
    header.stream_size = context.offset;
    header.file_count = context.file_count;
    header.dir_count = context.dir_count;
    header.root_file_count = (uint8_t)root->dir_num;
    header.root_dir_count = (uint8_t)root->dir_dnum;

    memcpy(buffer, &header, sizeof(header));
    return context.offset;
}

uint8_t fm_unpack(
    FMv3_record *reco,
    const uint8_t *buffer,
    uint32_t size)
{
    if (reco == NULL || reco->root == NULL || reco->dir_root == NULL ||
        buffer == NULL || size < sizeof(fm_pack_header))
    {
        return FALSE;
    }

    fm_pack_header header;
    memcpy(&header, buffer, sizeof(header));
    if (header.magic != FM_PACK_MAGIC ||
        header.version != FM_PACK_VERSION ||
        header.header_size != sizeof(header) ||
        header.stream_size != size ||
        header.file_count >= MAX_FILE_NUM ||
        header.dir_count >= MAX_DIR_NUM - 1 ||
        header.root_file_count > MAX_DIR_FILE_NUM ||
        header.root_dir_count > MAX_DIR_FILE_NUM)
    {
        return FALSE;
    }

    fcb_e *root = fm_dir_find(reco, "root");
    if (root == NULL || reco->all_file_num != 1 ||
        reco->all_dir_num != 2 || root->dir_num != 0 ||
        root->dir_dnum != 0)
    {
        return FALSE;
    }

    fm_unpack_context validation;
    memset(&validation, 0, sizeof(validation));
    validation.buffer = buffer;
    validation.size = size;
    validation.offset = sizeof(header);
    if (!fm_unpack_chk_contents(
            &validation,
            header.root_file_count,
            header.root_dir_count,
            0) ||
        validation.offset != size ||
        validation.file_count != header.file_count ||
        validation.dir_count != header.dir_count)
    {
        return FALSE;
    }

    fm_unpack_context context;
    memset(&context, 0, sizeof(context));
    context.reco = reco;
    context.buffer = buffer;
    context.size = size;
    context.offset = sizeof(header);

    if (!fm_unpack_dir_contents(
            &context,
            root,
            header.root_file_count,
            header.root_dir_count,
            0))
    {
        return FALSE;
    }

    return (uint8_t)(context.offset == size &&
                     context.file_count == header.file_count &&
                     context.dir_count == header.dir_count);
}

uint8_t fm_dsk_ready(void)
{
    return fm_dsk_active;
}

uint8_t fm_dsk_retire(uint32_t first_sector, uint32_t byte_length)
{
    if (byte_length == 0)
    {
        return (uint8_t)(first_sector == AXV_EMPTY);
    }
    if (!fm_dsk_active ||
        !axv_check_chain(first_sector, byte_length))
    {
        return FALSE;
    }

    fm_dsk_pending *pending =
        (fm_dsk_pending *)heap_alloc(sizeof(*pending));
    if (pending == NULL)
    {
        return FALSE;
    }

    pending->first_sector = first_sector;
    pending->byte_length = byte_length;
    pending->released = FALSE;
    pending->next = NULL;
    if (fm_dsk_pending_tail == NULL)
    {
        fm_dsk_pending_head = pending;
    }
    else
    {
        fm_dsk_pending_tail->next = pending;
    }
    fm_dsk_pending_tail = pending;
    return TRUE;
}

void fm_dsk_cancel_retire(uint32_t first_sector, uint32_t byte_length)
{
    fm_dsk_pending **link = &fm_dsk_pending_head;
    fm_dsk_pending *previous = NULL;
    while (*link != NULL)
    {
        fm_dsk_pending *pending = *link;
        if (!pending->released &&
            pending->first_sector == first_sector &&
            pending->byte_length == byte_length)
        {
            *link = pending->next;
            if (fm_dsk_pending_tail == pending)
            {
                fm_dsk_pending_tail = previous;
            }
            heap_free(pending);
            return;
        }
        previous = pending;
        link = &pending->next;
    }
}

static uint8_t fm_dsk_files_valid(FMv3_record *reco)
{
    if (reco == NULL || reco->all_file_num > MAX_FILE_NUM)
    {
        return FALSE;
    }
    for (uint16_t i = 1; i < reco->all_file_num; i++)
    {
        fcb_t *file = &reco->FMv3_mem[i];
        if (!file->is_alloc || file->fid != i ||
            !axv_check_chain(file->dsk_id, file->lens))
        {
            return FALSE;
        }
        if (file->dsk_id != AXV_EMPTY)
        {
            for (uint16_t j = 1; j < i; j++)
            {
                if (reco->FMv3_mem[j].dsk_id == file->dsk_id)
                {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

static uint8_t fm_dsk_reco_empty(FMv3_record *reco)
{
    fcb_e *root;
    if (reco == NULL || reco->all_file_num != 1 ||
        reco->all_dir_num != 2)
    {
        return FALSE;
    }
    root = fm_dir_find(reco, "root");
    return (uint8_t)(root != NULL &&
                     root->dir_num == 0 &&
                     root->dir_dnum == 0);
}

uint8_t fm_dsk_mount(FMv3_record *reco)
{
    uint32_t first_sector;
    uint32_t byte_length;

    if (reco == NULL)
    {
        return FALSE;
    }
    if (fm_dsk_active)
    {
        return TRUE;
    }
    if (!dsk_fs_mount_vblk())
    {

        return FALSE;
    }

    if (!axv_get_metadata(&first_sector, &byte_length))
    {
        axv_unmount();
        return FALSE;
    }
    if (!fm_dsk_reco_empty(reco))
    {
        puts("FM must be initialized and empty before mounting DSK\n");
        axv_unmount();
        return FALSE;
    }
    if (byte_length == 0)
    {
        fm_dsk_active = TRUE;
        fm_dsk_record = reco;
        return TRUE;
    }

    if (byte_length > sizeof(fm_dsk_buffer))
    {
        puts("DSK metadata is larger than the FM limit\n");
        axv_unmount();
        return FALSE;
    }

    if (axv_read(
            first_sector,
            byte_length,
            0,
            fm_dsk_buffer,
            byte_length) != byte_length ||
        !fm_unpack(reco, fm_dsk_buffer, byte_length) ||
        !fm_dsk_files_valid(reco))
    {
        puts("Cannot restore FM metadata from DSK\n");
        fm_init((uint64_t *)reco->base);
        axv_unmount();
        return FALSE;
    }

    fm_dsk_active = TRUE;
    fm_dsk_record = reco;
    return TRUE;
}

uint8_t fm_dsk_sync(void)
{
    uint32_t first_sector;
    uint32_t byte_length;
    uint32_t packed_size;
    uint32_t new_sector;

    if (!fm_dsk_active || fm_dsk_record == NULL ||
        !axv_get_metadata(&first_sector, &byte_length))
    {
        return FALSE;
    }

    packed_size = fm_pack(
        fm_dsk_record,
        fm_dsk_buffer,
        sizeof(fm_dsk_buffer));
    if (packed_size == 0)
    {
        puts("Cannot serialize FM metadata\n");
        return FALSE;
    }

    new_sector = axv_create(packed_size);
    if (new_sector == AXV_EMPTY)
    {
        puts("Cannot allocate a new DSK metadata chain\n");
        return FALSE;
    }
    if (axv_write(
            new_sector,
            packed_size,
            0,
            fm_dsk_buffer,
            packed_size) != packed_size)
    {
        axv_delete(new_sector, packed_size);
        puts("Cannot write FM metadata to DSK\n");
        return FALSE;
    }

    if (byte_length != 0 &&
        !fm_dsk_retire(first_sector, byte_length))
    {
        axv_delete(new_sector, packed_size);
        puts("Cannot queue the previous DSK metadata chain for release\n");
        return FALSE;
    }
    if (!axv_set_metadata_chain(new_sector, packed_size))
    {
        if (byte_length != 0)
        {
            fm_dsk_cancel_retire(first_sector, byte_length);
        }
        axv_delete(new_sector, packed_size);
        puts("Cannot update the DSK metadata chain\n");
        return FALSE;
    }

    if (!axv_sync())
    {
        puts("Cannot commit DSK metadata\n");
        return FALSE;
    }

    for (fm_dsk_pending *pending = fm_dsk_pending_head;
         pending != NULL;
         pending = pending->next)
    {
        if (pending->released)
        {
            continue;
        }
        if (!axv_check_chain(pending->first_sector, pending->byte_length) ||
            !axv_delete(pending->first_sector, pending->byte_length))
        {
            puts("Cannot release a superseded DSK chain\n");
            return FALSE;
        }
        pending->released = TRUE;
    }

    if (fm_dsk_pending_head != NULL && !axv_sync())
    {
        puts("Cannot persist released DSK chains\n");
        return FALSE;
    }

    while (fm_dsk_pending_head != NULL)
    {
        fm_dsk_pending *pending = fm_dsk_pending_head;
        fm_dsk_pending_head = pending->next;
        heap_free(pending);
    }
    fm_dsk_pending_tail = NULL;
    return TRUE;
}

uint8_t fm_dsk_unmount(void)
{
    if (!fm_dsk_active || !fm_dsk_sync())
    {
        return FALSE;
    }
    if (!axv_unmount())
    {
        puts("Cannot unmount DSK\n");
        return FALSE;
    }
    fm_dsk_active = FALSE;
    fm_dsk_record = NULL;
    return TRUE;
}