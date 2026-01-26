#include "startup.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usb_host.h"
#include "usbh_hid.h"
#include "usbd_conf.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
#include "usbd_cdc_if.h"

#define CDC_CLASS_ID 0
#define HID_CLASS_ID 1

USBD_HandleTypeDef hUsbDeviceFS;
static bool receivedCDC = false;
extern "C" void USB_DEVICE_Init();

static HID_KEYBD_Info_TypeDef last_info;
static uint8_t last_report[8];
static bool receivedKey = false;

uint8_t cdc_ep[3] = { CDC_IN_EP, CDC_OUT_EP, CDC_CMD_EP };
uint8_t hid_ep[1] = { HID_EPIN_ADDR };

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
    HID_KEYBD_Info_TypeDef* info = USBH_HID_GetKeybdInfo(phost);
    if (info == nullptr)
    {
    	return;
    }

    uint8_t report[8];
    build_hid_report(info, report);

    // Send HID report to PC if changed
    if (memcmp(report, last_report, 8) != 0)
    {
        memcpy(last_report, report, 8);
        last_info = *info;
        receivedKey = true;
    }
}

extern "C" void initialize()
{
}

extern "C" void setup()
{
	USB_DEVICE_Init();
	HAL_TIM_Base_Start_IT(&htim3);
}

extern "C" void loop()
{
	if (receivedKey)
	{
		USBD_HID_HandleTypeDef* hhid = (USBD_HID_HandleTypeDef*)hUsbDeviceFS.pClassDataCmsit[HID_CLASS_ID];
		if (hhid->state != USBD_HID_IDLE)
		{
			return;
		}

		receivedKey = false;

		// Send report to HID keyboard
        USBD_HID_SendReport(&hUsbDeviceFS, last_report, 8, HID_CLASS_ID);
    	HAL_Delay(5);

		// Send key text to CDC
		char cdc_buf[32];
		cdc_buf[0] = USBH_HID_GetASCIICode(&last_info);
		int len = 1;
		//int len = keycode_to_text(info, cdc_buf, sizeof(cdc_buf));
		if (len > 0)
		{
			CDC_Transmit_FS((uint8_t*)cdc_buf, len);
		}
	}

	//HAL_Delay(1000);
}

extern "C" void OnCdcReceive(uint8_t* Buf, uint32_t Len)
{
	receivedCDC = true;
}

extern "C" void USB_DEVICE_Init()
{
	if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
	{
		Error_Handler();
	}

	// CDC
	if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK)
	{
		Error_Handler();
	}
	if (USBD_RegisterClassComposite(&hUsbDeviceFS, &USBD_CDC, CLASS_TYPE_CDC, cdc_ep) != USBD_OK)
	{
		Error_Handler();
	}

	// HID
	if (USBD_RegisterClassComposite(&hUsbDeviceFS, &USBD_HID, CLASS_TYPE_HID, hid_ep) != USBD_OK)
	{
		Error_Handler();
	}

	if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
	{
		Error_Handler();
	}
}
