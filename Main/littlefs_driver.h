#ifndef LITTLEFS_DRIVER_H
#define LITTLEFS_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lfs.h"

/**
 * @brief  Initialize STM32 internal flash as LittleFS block device.
 * @param  lfs_out: pointer to store LittleFS object pointer
 * @param  cfg_out: pointer to store LittleFS config pointer
 */
void littlefs_driver_init(lfs_t **lfs_out, struct lfs_config **cfg_out);

#ifdef __cplusplus
}
#endif

#endif /* LITTLEFS_DRIVER_H */
