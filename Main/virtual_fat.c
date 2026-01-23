#include "virtual_fat.h"
#include "flash_ops.h"
#include <string.h>

// FAT32 configuration for 832KB
#define SECTOR_SIZE     512
#define SECTOR_COUNT    1664      // 832KB / 512
#define FAT_SECTORS     63        // Calculated for 832KB
#define RESERVED_SECTORS 32
#define ROOT_DIR_SECTOR 127       // After reserved + FATs

static bool vfat_initialized = false;

void vfat_init(void) {
    // Initialize flash if needed
    vfat_initialized = true;
}

uint32_t vfat_get_sector_count(void) {
    return SECTOR_COUNT;
}

bool vfat_is_ready(void) {
    return vfat_initialized;
}

static void generate_boot_sector(uint8_t *buf) {
    memset(buf, 0, SECTOR_SIZE);

    // Boot sector structure
    buf[0] = 0xEB; buf[1] = 0x3C; buf[2] = 0x90;
    memcpy(&buf[3], "STM32VFAT", 8);

    // BPB
    *(uint16_t*)&buf[11] = 512;
    buf[13] = 8;
    *(uint16_t*)&buf[14] = 32;
    buf[16] = 2;
    *(uint16_t*)&buf[19] = SECTOR_COUNT;
    buf[21] = 0xF8;
    *(uint32_t*)&buf[36] = FAT_SECTORS;
    *(uint32_t*)&buf[44] = 2;

    // Volume info
    buf[66] = 0x29;
    *(uint32_t*)&buf[67] = 0x12345678;
    memcpy(&buf[71], "STM32F407   ", 11);
    memcpy(&buf[82], "FAT32   ", 8);

    // Boot signature
    buf[510] = 0x55;
    buf[511] = 0xAA;
}

static void generate_fat_sector(uint8_t *buf, uint32_t sector_num) {
    uint32_t *fat = (uint32_t*)buf;
    uint32_t entries_per_sector = 128;

    for (uint32_t i = 0; i < entries_per_sector; i++) {
        uint32_t cluster = sector_num * entries_per_sector + i;

        if (cluster == 0) fat[i] = 0x0FFFFFF8;
        else if (cluster == 1) fat[i] = 0xFFFFFFFF;
        else if (cluster == 2) fat[i] = 0x0FFFFFFF;  // Root EOF
        else if (cluster == 3) fat[i] = 0x0FFFFFFF;  // README.TXT EOF
        else fat[i] = 0x00000000;  // Free
    }
}

bool vfat_read_sector(uint32_t sector, uint8_t *buffer) {
    if (sector == 0) {
        generate_boot_sector(buffer);
    }
    else if (sector < 1 + FAT_SECTORS) {
        generate_fat_sector(buffer, sector - 1);
    }
    else if (sector < 1 + 2*FAT_SECTORS) {
        generate_fat_sector(buffer, sector - 1 - FAT_SECTORS);
    }
    else if (sector == ROOT_DIR_SECTOR) {
        // Root directory with README.TXT
        memset(buffer, 0, SECTOR_SIZE);
        memcpy(&buffer[0], ".          ", 11); buffer[11] = 0x10;
        memcpy(&buffer[32], "..         ", 11); buffer[43] = 0x10;
        memcpy(&buffer[64], "README  TXT", 11); buffer[75] = 0x20;
        *(uint16_t*)&buffer[90] = 3;  // Start cluster
        *(uint32_t*)&buffer[92] = 52; // File size
    }
    else if (sector >= ROOT_DIR_SECTOR + 1) {
        // Data area - read from flash or generate
        uint32_t data_sector = sector - (ROOT_DIR_SECTOR + 1);

        if (data_sector == 0) {
            // README.TXT content
            const char *txt = "STM32F407 Virtual FAT USB Disk\n";
            uint32_t len = strlen(txt);
            memcpy(buffer, txt, len > 512 ? 512 : len);
        } else {
            // Read actual flash content
            uint32_t flash_addr = 0x08030000 + (sector * 512);
            flash_read_sector(flash_addr, buffer);
        }
    }

    return true;
}

bool vfat_write_sector(uint32_t sector, const uint8_t *buffer) {
    // Ignore writes to system areas
    if (sector < ROOT_DIR_SECTOR + 1) {
        return true;  // Success but ignored
    }

    // Write to actual flash
    uint32_t flash_addr = 0x08030000 + (sector * 512);
    return flash_write_sector(flash_addr, buffer);
}
