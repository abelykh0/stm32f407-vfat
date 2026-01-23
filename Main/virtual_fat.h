#ifndef VIRTUAL_FAT_H
#define VIRTUAL_FAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Virtual FAT API
void vfat_init(void);
bool vfat_read_sector(uint32_t sector, uint8_t *buffer);
bool vfat_write_sector(uint32_t sector, const uint8_t *buffer);
uint32_t vfat_get_sector_count(void);
bool vfat_is_ready(void);

// FAT32 constants
#define VFAT_SECTOR_SIZE  512
#define VFAT_SECTOR_COUNT 1664  // 832KB / 512

#ifdef __cplusplus
}
#endif

#endif /* VIRTUAL_FAT_H */
