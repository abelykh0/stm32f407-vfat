#include "startup.h"
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "littlefs_driver.h"
#include "mimic_fat.h"         // FAT emulation

static lfs_t *lfs;
static struct lfs_config *cfg;
extern lfs_file_t fat_cache;

extern "C" void initialize()
{
}

extern "C" void setup()
{
	littlefs_driver_init(&lfs, &cfg);
	mimic_fat_init(cfg);

//    int err = lfs_file_open(lfs, &fat_cache, ".mimic/FAT", LFS_O_RDWR);
//    int32_t size = lfs_file_size(lfs, &fat_cache);
	//mimic_fat_create_cache();


	MX_USB_DEVICE_Init();
}

extern "C" void loop()
{
}
