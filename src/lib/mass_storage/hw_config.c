/*
 *	dusk@initial version
 */

#include "stm32f10x.h"
#include "hw_config.h"
#include "mass_mal.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_lib.h"

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

void Get_SerialNum(void)
{
	uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

	Device_Serial0 = *(volatile uint32_t*)(0x1FFFF7E8);
	Device_Serial1 = *(volatile uint32_t*)(0x1FFFF7EC);
	Device_Serial2 = *(volatile uint32_t*)(0x1FFFF7F0);

	Device_Serial0 += Device_Serial2;

	if (Device_Serial0 != 0) {
		IntToUnicode (Device_Serial0, &MASS_StringSerial[2] , 8);
		IntToUnicode (Device_Serial1, &MASS_StringSerial[18], 4);
	}
}

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
	uint8_t idx = 0;
  
	for (idx = 0; idx < len; idx ++) {
		if( ((value >> 28)) < 0xA )
			pbuf[ 2* idx] = (value >> 28) + '0';
		else
			pbuf[2* idx] = (value >> 28) + 'A' - 10;

		value = value << 4;
		pbuf[ 2* idx + 1] = 0;
	}
}
