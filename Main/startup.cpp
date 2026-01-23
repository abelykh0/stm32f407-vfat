#include "startup.h"
#include "stm32f4xx_hal.h"
#include "virtual_fat.h"

extern "C" void initialize()
{
}

extern "C" void setup()
{
	vfat_init();
}

extern "C" void loop()
{
}
