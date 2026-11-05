#ifndef __KERNEL_DSK_FS_H__
#define __KERNEL_DSK_FS_H__

#include "_types.h"

#define AXV_SECTOR_SIZE 512U
#define AXV_EMPTY ((uint32_t)-1)

typedef uint8_t (*axv_sector_read_fn)(
    void *context,
    uint32_t sector,
    void *buffer);
typedef uint8_t (*axv_sector_write_fn)(
    void *context,
    uint32_t sector,
    const void *buffer);

uint8_t axv_format(
    void *io_context,
    axv_sector_read_fn read_sector,
    axv_sector_write_fn write_sector);
uint8_t axv_mount(
    void *io_context,
    axv_sector_read_fn read_sector,
    axv_sector_write_fn write_sector);
uint8_t axv_unmount(void);
uint8_t axv_set_metadata_chain(uint32_t first_sector, uint32_t byte_length);
uint32_t axv_create(uint32_t byte_length);
uint8_t axv_delete(uint32_t first_sector, uint32_t byte_length);
uint32_t axv_find(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    uint32_t *sector_offset_out);
uint32_t axv_read(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    void *buffer,
    uint32_t size);
uint32_t axv_write(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    const void *buffer,
    uint32_t size);

/* Explicitly formats the filesystem metadata area on the attached VirtIO disk. */
uint8_t dsk_fs_format_vblk(void);
uint8_t dsk_fs_mount_vblk(void);

#endif
