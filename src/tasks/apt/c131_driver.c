/*
 * David@2011 init version
 */
#include <stdio.h>
#include <string.h>
#include "spi.h"
#include "config.h"
#include "ulp_time.h"
#include "stm32f10x.h"
#include "mbi5025.h"
#include "c131_driver.h"

//Local Static RAM define
static unsigned char LoadRAM[8];
static unsigned short * pLoadRAM;
static unsigned char SRImage[6];

//Local device define
static mbi5025_t sr = {
	.bus = &spi1,
	.load_pin = SPI_CS_PC3,
	.oe_pin = SPI_CS_PC4,
};

void c131_driver_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//PD Port Init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD,  &GPIO_InitStructure);

	//PC Port Init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//PE Port Init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//for sdm detect input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//power set
	Disable_SDMPWR();
	Disable_SDMEXTPWR();
	Disable_LEDPWR();
	Disable_LEDEXTPWR();

	//spi device init
	mbi5025_Init(&sr);
	mbi5025_DisableLoad(&sr);
	mbi5025_EnableOE(&sr);

	//ram pointer init
	pLoadRAM = (unsigned short *)LoadRAM;
}

int c131_SetRelayRAM(unsigned char * pload)
{
	memcpy(LoadRAM, pload, 8);
	return 0;
}

int c131_relay_Update(void)
{
	memset(SRImage, 0x00, sizeof(SRImage));

	SRImage[1] = LoadRAM[2];
	SRImage[2] = LoadRAM[4];
	SRImage[3] = LoadRAM[6];
	SRImage[4] = LoadRAM[1];
	SRImage[5] = LoadRAM[0];

	mbi5025_WriteBytes(&sr, SRImage, sizeof(SRImage));

	return 0;
}

int loop_SetRelayStatus(unsigned short loop_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[0] &= ~loop_relays;
	} else {
		pLoadRAM[0] |= loop_relays;
	}
	return 0;
}

int loop_GetRelayStatus(unsigned short * ploop_status)
{
	(* ploop_status) = pLoadRAM[0];
	return 0;
}

int ess_SetRelayStatus(unsigned short ess_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[1] &= ~ess_relays;
	} else {
		pLoadRAM[1] |= ess_relays;
	}
	return 0;
}

int ess_GetRelayStatus(unsigned short * pess_status)
{
	(* pess_status) = pLoadRAM[1];
	return 0;
}

int led_SetRelayStatus(unsigned short led_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[2] &= ~led_relays;
	} else {
		pLoadRAM[2] |= led_relays;
	}
	return 0;
}

int led_GetRelayStatus(unsigned short * pled_status)
{
	(* pled_status) = pLoadRAM[2];
	return 0;
}

int sw_SetRelayStatus(unsigned short sw_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[3] &= ~sw_relays;
	} else {
		pLoadRAM[3] |= sw_relays;
	}
	return 0;
}

int sw_GetRelayStatus(unsigned short * psw_status)
{
	(* psw_status) = pLoadRAM[3];
	return 0;
}
