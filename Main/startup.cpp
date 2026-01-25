#include "startup.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usbh_hid.h"
#include "usb_host.h"
#include "usbd_hid.h"

#define HID_CLASS_ID 0
#define CDC_CLASS_ID 1

extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t last_report[8];

static void build_hid_report(HID_KEYBD_Info_TypeDef *info, uint8_t *report)
{
    memset(report, 0, 8);

    // Modifiers
    if(info->lctrl) report[0] |= 0x01;
    if(info->lshift) report[0] |= 0x02;
    if(info->lalt) report[0] |= 0x04;
    if(info->lgui) report[0] |= 0x08;
    if(info->rctrl) report[0] |= 0x10;
    if(info->rshift) report[0] |= 0x20;
    if(info->ralt) report[0] |= 0x40;
    if(info->rgui) report[0] |= 0x80;

    // Keycodes (up to 6)
    memcpy(&report[2], info->keys, 6);
}

extern "C" void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
{
    HID_KEYBD_Info_TypeDef *info = USBH_HID_GetKeybdInfo(phost);
    if (info == nullptr)
    {
    	return;
    }

    uint8_t report[8];
    build_hid_report(info, report);

    // Send HID report to PC if changed
    if (memcmp(report, last_report, 8) != 0)
    {
        USBD_HID_SendReport(&hUsbDeviceFS, report, 8, HID_CLASS_ID);
        memcpy(last_report, report, 8);
    }

    // Send key text to CDC
    //int len = keycode_to_text(info, cdc_buf, sizeof(cdc_buf));
    //char cdc_buf[32];
    //if (len > 0)
    //{
    //    while(CDC_Transmit_FS((uint8_t*)cdc_buf, len) == USBD_BUSY);
    //}
}





extern "C" void initialize()
{
}

extern "C" void setup()
{
	HAL_TIM_Base_Start_IT(&htim3);
}

extern "C" void loop()
{
}
