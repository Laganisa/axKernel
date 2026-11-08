#include "_types.h"
#include "global/_io.h"
#include "tools/_dsk_fs.h"
#include "tools/_virtio.h"

#define AXV_DISK_SIZE (64ULL * 1024 * 1024)
#define AXV_SECTOR_COUNT ((uint32_t)(AXV_DISK_SIZE / AXV_SECTOR_SIZE))
#define AXV_BITMAP_BYTES ((AXV_SECTOR_COUNT + 7U) / 8U)
#define AXV_BITMAP_WORDS ((AXV_SECTOR_COUNT + 63U) / 64U)
#define AXV_BITMAP_SECTORS ((AXV_BITMAP_BYTES + AXV_SECTOR_SIZE - 1) / AXV_SECTOR_SIZE)
#define AXV_SLL_BYTES ((uint64_t)AXV_SECTOR_COUNT * sizeof(uint32_t))
#define AXV_SLL_SECTORS ((AXV_SLL_BYTES + AXV_SECTOR_SIZE - 1) / AXV_SECTOR_SIZE)
#define AXV_SUPERBLOCK_SECTOR 0U
#define AXV_BITMAP_START 1U
#define AXV_SLL_START (AXV_BITMAP_START + AXV_BITMAP_SECTORS)
#define AXV_DATA_START (AXV_SLL_START + AXV_SLL_SECTORS)
#define AXV_TREE_LEAF_BASE AXV_BITMAP_WORDS
#define AXV_TREE_SIZE (AXV_TREE_LEAF_BASE * 2U)
#define AXV_VERSION 1U
#define AXV_MAGIC 0x31534644585641ULL

typedef struct axv_superblock
{
    uint64_t magic;
    uint32_t version;
    uint32_t sector_size;
    uint32_t sector_count;
    uint32_t bitmap_start;
    uint32_t bitmap_sectors;
    uint32_t sll_start;
    uint32_t sll_sectors;
    uint32_t data_start;
    uint32_t metadata_start;
    uint32_t metadata_bytes;
} axv_superblock;

static uint64_t axv_bitmap[AXV_BITMAP_WORDS]
    __attribute__((aligned(4096)));
static uint64_t axv_metadata_bitmap[AXV_BITMAP_WORDS];
static uint32_t axv_sll[AXV_SECTOR_COUNT]
    __attribute__((aligned(4096)));
static uint32_t axv_tree[AXV_TREE_SIZE];
static axv_superblock axv_sb;
static axv_sector_read_fn axv_read_sector;
static axv_sector_write_fn axv_write_sector;
static void *axv_io_context;
static uint8_t axv_is_mounted;

static void axv_copy(void *destination, const void *source, uint32_t size)
{
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;

    for (uint32_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
}

static uint8_t axv_sector_is_allocated(uint32_t sector)
{
    return (uint8_t)((axv_bitmap[sector / 64] >> (sector % 64)) & 1ULL);
}

static void axv_sector_set_allocated(uint32_t sector, uint8_t allocated)
{
    uint64_t mask = 1ULL << (sector % 64);
    if (allocated)
    {
        axv_bitmap[sector / 64] |= mask;
    }
    else
    {
        axv_bitmap[sector / 64] &= ~mask;
    }
}

static uint32_t axv_bitmap_find(uint32_t word_index)
{
    uint64_t free_bits = ~axv_bitmap[word_index];
    if (free_bits == 0)
    {
        return AXV_EMPTY;
    }

    uint32_t sector = word_index * 64U + (uint32_t)__builtin_ctzll(free_bits);
    return sector < AXV_SECTOR_COUNT ? sector : AXV_EMPTY;
}

static uint32_t axv_tree_min(uint32_t left, uint32_t right)
{
    if (left == AXV_EMPTY)
    {
        return right;
    }
    if (right == AXV_EMPTY)
    {
        return left;
    }
    return left < right ? left : right;
}

static void axv_tree_update(uint32_t word_index)
{
    uint32_t node = AXV_TREE_LEAF_BASE + word_index;
    axv_tree[node] = axv_bitmap_find(word_index);

    while (node > 1)
    {
        node /= 2;
        axv_tree[node] =
            axv_tree_min(axv_tree[node * 2], axv_tree[node * 2 + 1]);
    }
}

static void axv_tree_build(void)
{
    for (uint32_t i = 0; i < AXV_BITMAP_WORDS; i++)
    {
        axv_tree[AXV_TREE_LEAF_BASE + i] = axv_bitmap_find(i);
    }

    for (uint32_t i = AXV_TREE_LEAF_BASE - 1; i > 0; i--)
    {
        axv_tree[i] = axv_tree_min(axv_tree[i * 2], axv_tree[i * 2 + 1]);
    }
}

static uint8_t axv_superblock_valid(const axv_superblock *sb)
{

    uint8_t ret = (sb->magic == AXV_MAGIC &&
                   sb->version == AXV_VERSION &&
                   sb->sector_size == AXV_SECTOR_SIZE &&
                   sb->sector_count == AXV_SECTOR_COUNT &&
                   sb->bitmap_start == AXV_BITMAP_START &&
                   sb->bitmap_sectors == AXV_BITMAP_SECTORS &&
                   sb->sll_start == AXV_SLL_START &&
                   sb->sll_sectors == AXV_SLL_SECTORS &&
                   sb->data_start == AXV_DATA_START &&
                   sb->data_start < AXV_SECTOR_COUNT &&
                   ((sb->metadata_start == AXV_EMPTY && sb->metadata_bytes == 0) ||
                    (sb->metadata_start >= sb->data_start &&
                     sb->metadata_start < AXV_SECTOR_COUNT &&
                     sb->metadata_bytes != 0)));

    return ret;
}

static uint8_t axv_write_superblock(void)
{
    uint8_t sector[AXV_SECTOR_SIZE] __attribute__((aligned(16)));
    memset(sector, 0, sizeof(sector));
    axv_copy(sector, &axv_sb, sizeof(axv_sb));

    uint8_t ret = axv_write_sector(
        axv_io_context,
        AXV_SUPERBLOCK_SECTOR,
        sector);

    return ret;
}

static uint8_t axv_flush_metadata(void)
{
    const uint8_t *bitmap_bytes = (const uint8_t *)axv_bitmap;
    const uint8_t *sll_bytes = (const uint8_t *)axv_sll;

    for (uint32_t i = 0; i < AXV_SLL_SECTORS; i++)
    {
        if (!axv_write_sector(
                axv_io_context,
                AXV_SLL_START + i,
                sll_bytes + (uint64_t)i * AXV_SECTOR_SIZE))
        {

            return FALSE;
        }
    }

    for (uint32_t i = 0; i < AXV_BITMAP_SECTORS; i++)
    {
        if (!axv_write_sector(
                axv_io_context,
                AXV_BITMAP_START + i,
                bitmap_bytes + (uint64_t)i * AXV_SECTOR_SIZE))
        {
            return FALSE;
        }
    }

    return axv_write_superblock();
}

static uint8_t axv_load_metadata(void)
{

    uint8_t sector[AXV_SECTOR_SIZE] __attribute__((aligned(16)));

    if (!axv_read_sector(
            axv_io_context,
            AXV_SUPERBLOCK_SECTOR,
            sector))
    {

        return FALSE;
    }

    axv_copy(&axv_sb, sector, sizeof(axv_sb));
    if (!axv_superblock_valid(&axv_sb))
    {

        return FALSE;
    }

    uint8_t *bitmap_bytes = (uint8_t *)axv_bitmap;
    uint8_t *sll_bytes = (uint8_t *)axv_sll;
    for (uint32_t i = 0; i < AXV_BITMAP_SECTORS; i++)
    {
        if (!axv_read_sector(
                axv_io_context,
                AXV_BITMAP_START + i,
                bitmap_bytes + (uint64_t)i * AXV_SECTOR_SIZE))
        {

            return FALSE;
        }
    }

    for (uint32_t i = 0; i < AXV_SLL_SECTORS; i++)
    {
        if (!axv_read_sector(
                axv_io_context,
                AXV_SLL_START + i,
                sll_bytes + (uint64_t)i * AXV_SECTOR_SIZE))
        {

            return FALSE;
        }
    }

    for (uint32_t i = 0; i < AXV_DATA_START; i++)
    {
        if (!axv_sector_is_allocated(i))
        {

            return FALSE;
        }
    }

    axv_tree_build();
    return TRUE;
}

static uint8_t axv_chain_valid(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t *sector_count_out)
{
    if (first_sector == AXV_EMPTY || first_sector < AXV_DATA_START ||
        first_sector >= AXV_SECTOR_COUNT || byte_length == 0)
    {
        return FALSE;
    }

    uint32_t sector_count =
        byte_length / AXV_SECTOR_SIZE +
        (byte_length % AXV_SECTOR_SIZE != 0);
    if (sector_count == 0 || sector_count > AXV_SECTOR_COUNT - AXV_DATA_START)
    {
        return FALSE;
    }

    uint32_t sector = first_sector;
    for (uint32_t i = 0; i < sector_count; i++)
    {
        if (sector < AXV_DATA_START || sector >= AXV_SECTOR_COUNT ||
            !axv_sector_is_allocated(sector))
        {
            return FALSE;
        }

        uint32_t next = axv_sll[sector];
        if (i + 1 == sector_count)
        {
            if (next != AXV_EMPTY)
            {
                return FALSE;
            }
        }
        else if (next == AXV_EMPTY)
        {
            return FALSE;
        }
        sector = next;
    }

    if (sector_count_out != NULL)
    {
        *sector_count_out = sector_count;
    }
    return TRUE;
}

static void axv_metadata_bitmap_build(void)
{
    memset(axv_metadata_bitmap, 0, sizeof(axv_metadata_bitmap));
    if (axv_sb.metadata_bytes == 0)
    {
        return;
    }

    uint32_t sector_count =
        axv_sb.metadata_bytes / AXV_SECTOR_SIZE +
        (axv_sb.metadata_bytes % AXV_SECTOR_SIZE != 0);
    uint32_t sector = axv_sb.metadata_start;
    for (uint32_t i = 0; i < sector_count; i++)
    {
        axv_metadata_bitmap[sector / 64] |= 1ULL << (sector % 64);
        sector = axv_sll[sector];
    }
}

static void axv_release_chain(uint32_t first_sector, uint32_t sector_count)
{
    uint32_t sector = first_sector;
    for (uint32_t i = 0; i < sector_count; i++)
    {
        uint32_t next = axv_sll[sector];
        axv_sll[sector] = AXV_EMPTY;
        axv_sector_set_allocated(sector, FALSE);
        axv_tree_update(sector / 64);
        sector = next;
    }
}

uint8_t axv_format(
    void *io_context,
    axv_sector_read_fn read_sector,
    axv_sector_write_fn write_sector)
{

    if (read_sector == NULL || write_sector == NULL)
    {

        return FALSE;
    }

    axv_is_mounted = FALSE;
    axv_io_context = io_context;
    axv_read_sector = read_sector;
    axv_write_sector = write_sector;
    memset(axv_bitmap, 0, sizeof(axv_bitmap));
    memset(axv_metadata_bitmap, 0, sizeof(axv_metadata_bitmap));
    memset(axv_sll, 0xFF, sizeof(axv_sll));

    for (uint32_t i = 0; i < AXV_DATA_START; i++)
    {
        axv_sector_set_allocated(i, TRUE);
    }

    axv_sb.magic = AXV_MAGIC;
    axv_sb.version = AXV_VERSION;
    axv_sb.sector_size = AXV_SECTOR_SIZE;
    axv_sb.sector_count = AXV_SECTOR_COUNT;
    axv_sb.bitmap_start = AXV_BITMAP_START;
    axv_sb.bitmap_sectors = AXV_BITMAP_SECTORS;
    axv_sb.sll_start = AXV_SLL_START;
    axv_sb.sll_sectors = AXV_SLL_SECTORS;
    axv_sb.data_start = AXV_DATA_START;
    axv_sb.metadata_start = AXV_EMPTY;
    axv_sb.metadata_bytes = 0;

    axv_tree_build();
    axv_is_mounted = TRUE;
    if (!axv_flush_metadata())
    {
        log("???");

        axv_is_mounted = FALSE;
        return FALSE;
    }
    return TRUE;
}

uint8_t axv_mount(
    void *io_context,
    axv_sector_read_fn read_sector,
    axv_sector_write_fn write_sector)
{
    if (read_sector == NULL || write_sector == NULL)
    {
        return FALSE;
    }

    axv_is_mounted = FALSE;
    axv_io_context = io_context;
    axv_read_sector = read_sector;
    axv_write_sector = write_sector;
    if (!axv_load_metadata())
    {
        return FALSE;
    }

    if (axv_sb.metadata_bytes != 0 &&
        !axv_chain_valid(
            axv_sb.metadata_start,
            axv_sb.metadata_bytes,
            NULL))
    {
        return FALSE;
    }
    axv_metadata_bitmap_build();
    axv_is_mounted = TRUE;
    return TRUE;
}

uint8_t axv_unmount(void)
{
    if (!axv_is_mounted || axv_write_sector == NULL)
    {
        return FALSE;
    }
    if (!axv_flush_metadata())
    {
        return FALSE;
    }

    axv_is_mounted = FALSE;
    axv_io_context = NULL;
    axv_read_sector = NULL;
    axv_write_sector = NULL;
    return TRUE;
}

uint8_t axv_sync(void)
{
    return (uint8_t)(axv_is_mounted && axv_flush_metadata());
}

uint8_t axv_get_metadata(uint32_t *first_sector, uint32_t *byte_length)
{
    if (!axv_is_mounted || first_sector == NULL || byte_length == NULL)
    {
        return FALSE;
    }

    *first_sector = axv_sb.metadata_start;
    *byte_length = axv_sb.metadata_bytes;
    return TRUE;
}

uint8_t axv_set_metadata_chain(uint32_t first_sector, uint32_t byte_length)
{
    if (!axv_is_mounted)
    {
        return FALSE;
    }

    if (byte_length == 0)
    {
        if (first_sector != AXV_EMPTY)
        {
            return FALSE;
        }
    }
    else if (!axv_chain_valid(first_sector, byte_length, NULL))
    {
        return FALSE;
    }

    axv_sb.metadata_start = first_sector;
    axv_sb.metadata_bytes = byte_length;
    axv_metadata_bitmap_build();
    return TRUE;
}

uint8_t axv_check_chain(uint32_t first_sector, uint32_t byte_length)
{
    if (!axv_is_mounted)
    {
        return FALSE;
    }
    if (byte_length == 0)
    {
        return (uint8_t)(first_sector == AXV_EMPTY);
    }
    return axv_chain_valid(first_sector, byte_length, NULL);
}

uint32_t axv_create(uint32_t byte_length)
{
    if (!axv_is_mounted || byte_length == 0)
    {
        return AXV_EMPTY;
    }

    uint32_t required =
        byte_length / AXV_SECTOR_SIZE +
        (byte_length % AXV_SECTOR_SIZE != 0);
    uint32_t free_count = 0;

    for (uint32_t i = 0; i < AXV_BITMAP_WORDS; i++)
    {
        free_count += (uint32_t)__builtin_popcountll(~axv_bitmap[i]);
    }
    if (free_count < required)
    {
        return AXV_EMPTY;
    }

    uint32_t first_sector = AXV_EMPTY;
    uint32_t previous_sector = AXV_EMPTY;
    for (uint32_t i = 0; i < required; i++)
    {
        uint32_t sector = axv_tree[1];
        if (sector == AXV_EMPTY || sector < AXV_DATA_START ||
            sector >= AXV_SECTOR_COUNT)
        {
            if (first_sector != AXV_EMPTY)
            {
                axv_release_chain(first_sector, i);
            }
            return AXV_EMPTY;
        }

        axv_sector_set_allocated(sector, TRUE);
        axv_sll[sector] = AXV_EMPTY;
        if (previous_sector != AXV_EMPTY)
        {
            axv_sll[previous_sector] = sector;
        }
        else
        {
            first_sector = sector;
        }
        previous_sector = sector;
        axv_tree_update(sector / 64);
    }

    uint8_t zero_sector[AXV_SECTOR_SIZE] __attribute__((aligned(16)));
    memset(zero_sector, 0, sizeof(zero_sector));
    uint32_t sector = first_sector;
    for (uint32_t i = 0; i < required; i++)
    {
        if (!axv_write_sector(axv_io_context, sector, zero_sector))
        {
            axv_release_chain(first_sector, required);
            return AXV_EMPTY;
        }
        sector = axv_sll[sector];
    }

    return first_sector;
}

uint8_t axv_delete(uint32_t first_sector, uint32_t byte_length)
{
    if (!axv_is_mounted ||
        !axv_chain_valid(first_sector, byte_length, NULL))
    {
        return FALSE;
    }

    uint32_t sector_count =
        byte_length / AXV_SECTOR_SIZE +
        (byte_length % AXV_SECTOR_SIZE != 0);
    uint32_t sector = first_sector;
    for (uint32_t i = 0; i < sector_count; i++)
    {
        if ((axv_metadata_bitmap[sector / 64] >> (sector % 64)) & 1ULL)
        {
            return FALSE;
        }
        sector = axv_sll[sector];
    }

    axv_release_chain(first_sector, sector_count);
    return TRUE;
}

uint32_t axv_find(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    uint32_t *sector_offset_out)
{
    if (!axv_is_mounted || sector_offset_out == NULL ||
        byte_offset >= byte_length ||
        !axv_chain_valid(first_sector, byte_length, NULL))
    {
        return AXV_EMPTY;
    }

    uint32_t sector = first_sector;
    uint32_t hops = byte_offset / AXV_SECTOR_SIZE;
    for (uint32_t i = 0; i < hops; i++)
    {
        sector = axv_sll[sector];
        if (sector == AXV_EMPTY || sector >= AXV_SECTOR_COUNT)
        {
            return AXV_EMPTY;
        }
    }

    *sector_offset_out = byte_offset % AXV_SECTOR_SIZE;
    return sector;
}

static uint32_t axv_transfer(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    void *buffer,
    uint32_t size,
    uint8_t write)
{
    if (!axv_is_mounted || (buffer == NULL && size != 0) ||
        byte_offset > byte_length || size > byte_length - byte_offset)
    {
        return 0;
    }
    if (size == 0)
    {
        return 0;
    }

    uint32_t sector_offset;
    uint32_t sector = axv_find(
        first_sector,
        byte_length,
        byte_offset,
        &sector_offset);
    if (sector == AXV_EMPTY)
    {
        return 0;
    }

    uint8_t sector_buffer[AXV_SECTOR_SIZE] __attribute__((aligned(16)));
    uint8_t *bytes = (uint8_t *)buffer;
    uint32_t transferred = 0;
    while (transferred < size)
    {
        uint32_t chunk = AXV_SECTOR_SIZE - sector_offset;
        if (chunk > size - transferred)
        {
            chunk = size - transferred;
        }

        if (write)
        {
            if (chunk != AXV_SECTOR_SIZE &&
                !axv_read_sector(axv_io_context, sector, sector_buffer))
            {
                break;
            }
            axv_copy(
                sector_buffer + sector_offset,
                bytes + transferred,
                chunk);
            if (!axv_write_sector(axv_io_context, sector, sector_buffer))
            {
                break;
            }
        }
        else
        {
            if (!axv_read_sector(axv_io_context, sector, sector_buffer))
            {
                break;
            }
            axv_copy(
                bytes + transferred,
                sector_buffer + sector_offset,
                chunk);
        }

        transferred += chunk;
        sector_offset = 0;
        if (transferred < size)
        {
            sector = axv_sll[sector];
            if (sector == AXV_EMPTY || sector >= AXV_SECTOR_COUNT)
            {
                break;
            }
        }
    }

    return transferred;
}

uint32_t axv_read(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    void *buffer,
    uint32_t size)
{
    return axv_transfer(
        first_sector,
        byte_length,
        byte_offset,
        buffer,
        size,
        FALSE);
}

uint32_t axv_write(
    uint32_t first_sector,
    uint32_t byte_length,
    uint32_t byte_offset,
    const void *buffer,
    uint32_t size)
{
    return axv_transfer(
        first_sector,
        byte_length,
        byte_offset,
        (void *)buffer,
        size,
        TRUE);
}

static uint8_t dsk_fs_vblk_ready(void)
{
    blk_init();

    uint8_t ret = (uint8_t)(virtio_blk_is_ready() &&
                            virtio_blk_sector_count() >= AXV_SECTOR_COUNT);

    return ret;
}

static uint8_t dsk_fs_vblk_read(
    void *context,
    uint32_t sector,
    void *buffer)
{
    (void)context;
    return virtio_blk_read_sector(sector, buffer);
}

static uint8_t dsk_fs_vblk_write(
    void *context,
    uint32_t sector,
    const void *buffer)
{
    (void)context;
    return virtio_blk_write_sector(sector, buffer);
}

uint8_t dsk_fs_format_vblk(void)
{

    if (!dsk_fs_vblk_ready())
    {
        return FALSE;
    }

    return axv_format(
        NULL,
        dsk_fs_vblk_read,
        dsk_fs_vblk_write);
}

uint8_t dsk_fs_mount_vblk(void)
{
    if (!dsk_fs_vblk_ready())
    {

        return FALSE;
    }

    return axv_mount(
        NULL,
        dsk_fs_vblk_read,
        dsk_fs_vblk_write);
}
