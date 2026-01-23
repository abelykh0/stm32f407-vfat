#ifndef FLASH_OPS_H
#define FLASH_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Flash operations for USB MSC
bool flash_write_sector(uint32_t addr, const uint8_t *data);
bool flash_read_sector(uint32_t addr, uint8_t *data);
void flash_init(void);
uint32_t flash_get_storage_start(void);
uint32_t flash_get_storage_size(void);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_OPS_H */
