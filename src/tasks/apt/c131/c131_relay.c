/*
 * David@2011 init version
 */
 #include <stdio.h>
#include <string.h>
#include "c131_relay.h"
#include "spi.h"
#include "nvm.h"
#include "config.h"
#include "time.h"
#include "stm32f10x.h"
#include "mbi5025.h"

//load variant nvm
static c131_load_t c131_load[NUM_OF_LOAD] __nvm;
static int c131_current_load __nvm;

//Local Static RAM define
static unsigned char LoadRAM[8];
static unsigned short * pLoadRAM;
static unsigned char SRImage[6];

//Local Device define
static mbi5025_t sr = {
	.bus = &spi1,
	.load_pin = SPI_CS_PC3,
	.oe_pin = SPI_CS_PC4,
};

void c131_relay_Init(void)
{
	nvm_init();
	//load config from rom
	pLoadRAM = (unsigned short *)LoadRAM;

	//spi device init
	mbi5025_Init(&sr);
	mbi5025_DisableLoad(&sr);
	mbi5025_EnableOE(&sr);

	//init config which has been confirmed
	c131_current_load = 0;
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

int c131_GetLoad(c131_load_t ** pload, int index_load)
{
	if ((index_load >= 0) & (index_load < NUM_OF_LOAD)) {
		*pload = &c131_load[index_load];
		return 0;
	} else
		return -1;
}

int c131_AddLoad(c131_load_t * pload)
{
	int i;
	for (i = 0; i < NUM_OF_LOAD; i++) {
		if (c131_load[i].load_bExist == 0) {
			c131_load[i] = (*pload);
			c131_load[i].load_bExist =1;
			nvm_save();
			return 0;
		}
	}

	return 1;
}

int c131_ConfirmLoad(int index_load)
{
	c131_current_load = index_load;

	memcpy(LoadRAM, c131_load[c131_current_load].load_ram, 8);

	return 0;
}

int c131_GetCurrentLoadIndex(void)
{
	return c131_current_load;
}

