#include "startup.h"
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "littlefs_driver.h"
#include "mimic_fat.h"         // FAT emulation

static lfs_t *lfs;
static struct lfs_config *cfg;

extern "C" void initialize()
{
}

extern "C" void setup()
{
	littlefs_driver_init(&lfs, &cfg);
	mimic_fat_init(cfg);

	MX_USB_DEVICE_Init();
}

extern "C" void loop()
{
}
