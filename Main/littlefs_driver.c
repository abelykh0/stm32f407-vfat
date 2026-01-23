/* littlefs_driver.c
 *
 * STM32F407 internal flash LittleFS block device
 * Flash region: 0x08030000 – 0x080FFFFF (1 MB)
 * HAL-only implementation
 */

#include "lfs.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#define FLASH_START_ADDR  0x08030000
#define FLASH_END_ADDR    0x080FFFFF
#define FLASH_SIZE        (FLASH_END_ADDR - FLASH_START_ADDR + 1)
#define LFS_BLOCK_SIZE    4096     // align with STM32F4 flash sectors
#define LFS_CACHE_SIZE    512
#define LFS_LOOKAHEAD     16

static lfs_t lfs_global;
static struct lfs_config lfs_cfg_global;

// --- LittleFS block device callbacks ---

static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
                          lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint8_t *addr = (uint8_t *)(FLASH_START_ADDR + block * c->block_size + off);
    memcpy(buffer, addr, size);
    return 0;
}

static int lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
                          lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = FLASH_START_ADDR + block * c->block_size + off;
    const uint8_t *buf8 = (const uint8_t *)buffer;

    HAL_FLASH_Unlock();
    __disable_irq();

    // Program 32-bit words
    for (uint32_t i = 0; i < size; i += 4)
    {
        uint32_t data = *((uint32_t *)(buf8 + i));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, data) != HAL_OK)
        {
            __enable_irq();
            HAL_FLASH_Lock();
            return -1;
        }
    }

    __enable_irq();
    HAL_FLASH_Lock();
    return 0;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t sector_addr = FLASH_START_ADDR + block * c->block_size;

    // Determine STM32F407 sector number
    uint32_t sector = 0;

    if (sector_addr < 0x08040000) sector = 3;
    else if (sector_addr < 0x08060000) sector = 4;
    else if (sector_addr < 0x08080000) sector = 5;
    else if (sector_addr < 0x080A0000) sector = 6;
    else if (sector_addr < 0x080C0000) sector = 7;
    else if (sector_addr < 0x080E0000) sector = 8;
    else sector = 9;
    // Only one sector at a time
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t sector_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = sector;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();
    __disable_irq();

    if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK)
    {
        __enable_irq();
        HAL_FLASH_Lock();
        return -1;
    }

    __enable_irq();
    HAL_FLASH_Lock();
    return 0;
}

static int lfs_flash_sync(const struct lfs_config *c)
{
    return 0; // Nothing needed for internal flash
}

// --- Public API ---

void littlefs_driver_init(lfs_t **lfs_out, struct lfs_config **cfg_out)
{
    memset(&lfs_global, 0, sizeof(lfs_global));
    memset(&lfs_cfg_global, 0, sizeof(lfs_cfg_global));

    lfs_cfg_global.read  = lfs_flash_read;
    lfs_cfg_global.prog  = lfs_flash_prog;
    lfs_cfg_global.erase = lfs_flash_erase;
    lfs_cfg_global.sync  = lfs_flash_sync;

    lfs_cfg_global.block_size = LFS_BLOCK_SIZE;
    lfs_cfg_global.block_count = FLASH_SIZE / LFS_BLOCK_SIZE;
    lfs_cfg_global.cache_size = LFS_CACHE_SIZE;
    lfs_cfg_global.lookahead_size = LFS_LOOKAHEAD;

    *lfs_out = &lfs_global;
    *cfg_out = &lfs_cfg_global;

    // Try to mount, if fails format
    if (lfs_mount(&lfs_global, &lfs_cfg_global) != 0)
    {
        lfs_format(&lfs_global, &lfs_cfg_global);
        lfs_mount(&lfs_global, &lfs_cfg_global);
    }
}
