/*
 * David@2011 init version
 */
#include "c131_relay.h"
#include "spi.h"
#include "config.h"
#include "time.h"
#include "stm32f10x.h"
#include "mbi5025.h"
#include <stdio.h>
#include <string.h>

//Local Static RAM define
static unsigned char LoadRAM[8];static unsigned short pLoadRAM;
static unsigned char SRImage[6];

//Local Device define
static mbi5025_t sr = {
	.bus = &spi2,
	.load_pin = SPI_CS_PC3,
	.oe_pin = SPI_CS_PC4,
};

void c131_relay_Init(void)
{
	//load config from rom
	pLoadRAM = (unsigned short)LoadRAM;
	
	//spi device init
	mbi5025_Init(&sr);
	mbi5025_DisableLoad(&sr);
	mbi5025_EnableOE(&sr);
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
	return 0;}

int sw_GetRelayStatus(unsigned short * psw_status)
{
	(* psw_status) = pLoadRAM[3];
	return 0;
}
