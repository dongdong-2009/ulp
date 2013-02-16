/*
 * miaofng@2013-2-13 initial version
 */

#include "common/vchip.h"
#include <stddef.h>
#include <string.h>

static unsigned vchip_base;
void vchip_init(void *mmr)
{
	vchip_base = (unsigned) mmr;
}

void vchip_reset(vchip_t *vchip)
{
	vchip->flag_soh = 0;
	vchip->flag_esc = 0;
}

static inline void vchip_soh(vchip_t *vchip)
{
	vchip->flag_soh = 1;
	vchip->flag_esc = 0;
	vchip->flag_cmd = 0;
	vchip->n = 0;
}

void vchip_update(vchip_t *vchip)
{
	char byte;
	vchip->txd = NULL;

	if(vchip->rxd != NULL) {
		byte = *(vchip->rxd);
		if((byte == VCHIP_SOH) && (vchip->flag_esc == 0)) {
			vchip_soh(vchip);
			return;
		}
	}

	if(! vchip->flag_soh) {
		return;
	}

	if(vchip->rxd != NULL) {
		//esc process
		if((byte == VCHIP_ESC) && (vchip->flag_esc == 0)) {
			vchip->flag_esc = 1;
			return;
		}

		vchip->flag_esc = 0;
		if(vchip->flag_cmd) {
			vchip->dat[vchip->n ++] = byte;
		}
		else {
			vchip->cmd.value = byte;
			vchip->flag_cmd = 1;
		}
	}

	switch(vchip->cmd.typ) {
	case VCHIP_AR:
		if(vchip->n == 2) {
			unsigned short ofs = 0;
			memcpy(&ofs, vchip->dat, 2);
			vchip->adr = (char *) (vchip_base + ofs);
			vchip_reset(vchip);
			vchip->txd = 0x00;
		}
		break;
	case VCHIP_AA:
		if(vchip->n == 4) {
			unsigned adr;
			memcpy(&adr, vchip->dat, 4);
			vchip->adr = (char *) adr;
			vchip_reset(vchip);
			vchip->txd = 0x00;
		}
		break;
	case VCHIP_W:
		if(vchip->n == vchip->cmd.len) { //do write here
			memcpy(vchip->adr, vchip->dat, vchip->cmd.len);
			vchip_reset(vchip);
			vchip->txd = 0x00;
		}
		break;
	case VCHIP_R:
		if(vchip->n == 0) {
			memcpy(vchip->dat, vchip->adr, vchip->cmd.len);
		}

		if(vchip->n < vchip->cmd.len) {
			vchip->txd = vchip->dat + vchip->n;
		}
		break;
	default:;
	}
}
