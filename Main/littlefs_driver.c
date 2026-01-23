#include "littlefs_driver.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdbool.h>

/* ================= Flash layout ================= */
/* STM32F407 sectors:
   6..11 = 128KB each → 0x08040000 – 0x080FFFFF */

#define LFS_FLASH_START   0x08040000UL   /* Sector 6 */
#define LFS_FLASH_END     0x08100000UL   /* End of 1MB */
#define LFS_BLOCK_SIZE    (128 * 1024)   /* MUST match sector size */
#define LFS_BLOCK_COUNT   ((LFS_FLASH_END - LFS_FLASH_START) / LFS_BLOCK_SIZE)

/* ================= Static instances ================= */

static lfs_t lfs_inst;
static struct lfs_config cfg_inst;

lfs_t *lfs = &lfs_inst;
struct lfs_config *cfg = &cfg_inst;

/* ================= Helpers ================= */

static uint32_t block_to_sector(lfs_block_t block)
{
    /* sectors 6..11 */
    return FLASH_SECTOR_6 + block;
}

/* ================= Flash callbacks ================= */

static int flash_read(const struct lfs_config *c,
                      lfs_block_t block, lfs_off_t off,
                      void *buffer, lfs_size_t size)
{
    uint32_t addr = LFS_FLASH_START +
                    (block * c->block_size) + off;
    memcpy(buffer, (const void *)addr, size);
    return 0;
}

static int flash_prog(const struct lfs_config *c,
                      lfs_block_t block, lfs_off_t off,
                      const void *buffer, lfs_size_t size)
{
    uint32_t addr = LFS_FLASH_START +
                    (block * c->block_size) + off;

    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t data;
        memcpy(&data, (const uint8_t *)buffer + i, 4);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                              addr + i, data) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t err;

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = block_to_sector(block);
    erase.NbSectors    = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &err);
    HAL_FLASH_Lock();

    return (st == HAL_OK) ? 0 : -1;
}

static int flash_sync(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

/* ================= Public init ================= */

void littlefs_driver_init(lfs_t **out_lfs,
                          struct lfs_config **out_cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->read  = flash_read;
    cfg->prog  = flash_prog;
    cfg->erase = flash_erase;
    cfg->sync  = flash_sync;

    cfg->block_size      = LFS_BLOCK_SIZE;
    cfg->block_count     = LFS_BLOCK_COUNT;
    cfg->read_size       = 16;
    cfg->prog_size       = 4;
    cfg->cache_size      = 16;
    cfg->lookahead_size  = 16;
    cfg->block_cycles    = 500;

    if (lfs_mount(lfs, cfg) != 0) {
        lfs_format(lfs, cfg);
        lfs_mount(lfs, cfg);
    }

    if (out_lfs) *out_lfs = lfs;
    if (out_cfg) *out_cfg = cfg;
}
