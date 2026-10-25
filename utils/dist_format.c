// disk의 파일 시스템 포맷을 담당

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512
#define BITMAP_START 1
#define BITMAP_SECTORS 32
#define DATA_START 33
#define MAX_SECTORS (BITMAP_SECTORS * SECTOR_SIZE * 8)

#define MAGIC "AXFS0001"
#define VERSION 1
#define USAGE 1

static void put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static int write_sector(FILE *disk, uint32_t sector, const uint8_t *data)
{
    if (fseek(disk, (long)sector * SECTOR_SIZE, SEEK_SET) != 0)
        return -1;

    if (fwrite(data, 1, SECTOR_SIZE, disk) != SECTOR_SIZE)
        return -1;

    return 0;
}

static int format_disk(const char *path, uint32_t total_sectors)
{
    FILE *disk;
    uint8_t superblock[SECTOR_SIZE];
    uint8_t bitmap[BITMAP_SECTORS * SECTOR_SIZE];
    long image_size;

    if (total_sectors <= DATA_START || total_sectors > MAX_SECTORS)
        return -1;

    image_size = (long)total_sectors * SECTOR_SIZE;

    memset(superblock, 0, sizeof(superblock));
    memset(bitmap, 0, sizeof(bitmap));

    /* Superblock */
    memcpy(&superblock[0], MAGIC, 8);
    superblock[8] = VERSION;
    superblock[9] = USAGE;

    put_u32(&superblock[12], SECTOR_SIZE);
    put_u32(&superblock[16], total_sectors);
    put_u32(&superblock[20], BITMAP_START);
    put_u32(&superblock[24], BITMAP_SECTORS);
    put_u32(&superblock[28], DATA_START);
    put_u32(&superblock[32], total_sectors - DATA_START);

    /* Reserve sectors 0~32 */
    for (uint32_t i = 0; i < DATA_START; i++)
        bitmap[i / 8] |= (uint8_t)(1u << (i % 8));

    /* Create a new disk image */
    disk = fopen(path, "wb+");

    if (disk == NULL)
        return -1;

    /* Set image size without clearing the entire data area */
    if (fseek(disk, image_size - 1, SEEK_SET) != 0 ||
        fputc(0, disk) == EOF)
    {
        fclose(disk);
        return -1;
    }

    /* Write bitmap before publishing the superblock */
    if (fseek(disk, BITMAP_START * SECTOR_SIZE, SEEK_SET) != 0 ||
        fwrite(bitmap, 1, sizeof(bitmap), disk) != sizeof(bitmap))
    {
        fclose(disk);
        return -1;
    }

    if (write_sector(disk, 0, superblock) != 0)
    {
        fclose(disk);
        return -1;
    }

    if (fclose(disk) != 0)
        return -1;

    printf("Disk formatted successfully.\n");
    printf("Sectors: %u\n", total_sectors);
    printf("Size: %ld bytes\n", image_size);

    return 0;
}

int main(int argc, char **argv)
{
    char *end;
    unsigned long sectors;

    if (argc != 3)
    {
        printf("Usage: format <image> <sectors>\n");
        return 1;
    }

    sectors = strtoul(argv[2], &end, 10);

    if (*argv[2] == '\0' || *end != '\0' ||
        sectors > UINT32_MAX)
    {
        printf("Invalid sector count.\n");
        return 1;
    }

    if (format_disk(argv[1], (uint32_t)sectors) != 0)
    {
        printf("Disk format failed.\n");
        return 1;
    }

    return 0;
}
