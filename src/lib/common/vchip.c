/*
 * miaofng@2013-2-13 initial version
 */

#include "common/vchip.h"
#include <stddef.h>
#include <string.h>

enum {
	ECODE_OK = 0x01, /*note: 0x00 is the spi default output value*/
	ECODE_FAIL,
};

static char *vchip_ri;
static const char *vchip_ro;
static const char *vchip_az;
unsigned vchip_n = 0;

void vchip_init(void *ri, const void *ro, const void *az, int n)
{
	vchip_ri = ri;
	vchip_ro = ro;
	vchip_az = az;
	vchip_n = n;
}

void vchip_reset(vchip_t *vchip)
{
	vchip->flag_soh = 0;
	vchip->flag_esc = 0;
	vchip->txd = NULL;
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
	char byte, x, y, s;
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
		if(vchip->flag_cmd && (vchip->n < 14)) {
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
			vchip->ofs = ofs;
			vchip->flag_abs = 0;
			vchip_reset(vchip);
			vchip->ecode = ECODE_OK;
			vchip->txd = &vchip->ecode;
		}
		break;
	case VCHIP_AA:
		if(vchip->n == 4) {
			unsigned adr;
			memcpy(&adr, vchip->dat, 4);
			vchip->ofs = adr;
			vchip->flag_abs = 1;
			vchip_reset(vchip);
			vchip->ecode = ECODE_OK;
			vchip->txd = &vchip->ecode;
		}
		break;
	case VCHIP_W:
		if(vchip->n == vchip->cmd.len) { //do write here
			vchip->ecode = ECODE_OK;
			if(vchip->flag_abs) {
				memcpy((char *) vchip->ofs, vchip->dat, vchip->cmd.len);
			}
			else {
				if(vchip->ofs + vchip->n < vchip_n) {
					for(int i = 0; i < vchip->n; i ++) {
						x = vchip->dat[i];
						y = vchip_ri[vchip->ofs + i];
						s = vchip_az[vchip->ofs + i];
						byte = ((x ^ y) & s) | (x & (~ s));
						vchip_ri[vchip->ofs + i] = byte;
					}
				}
				else vchip->ecode = ECODE_FAIL;
			}
			vchip_reset(vchip);
			vchip->txd = &vchip->ecode;
		}
		break;
	case VCHIP_R:
		if(vchip->n == 0) {
			if(vchip->flag_abs) {
				memcpy(vchip->dat, (char *) vchip->ofs, vchip->cmd.len);
			}
			else {
				if(vchip->ofs + vchip->cmd.len < vchip_n) {
					for(int i = 0; i < vchip->cmd.len; i ++) {
						x = vchip_ro[vchip->ofs + i];
						y = vchip_ri[vchip->ofs + i];
						s = vchip_az[vchip->ofs + i];
						byte = ((x ^ y) & s) | (x & (~ s));
						vchip->dat[i] = byte;
					}
				}
				else {
					memset(vchip->dat, 0xff, vchip->cmd.len);
				}
			}
		}

		if(vchip->n < vchip->cmd.len) {
			vchip->txd = vchip->dat + vchip->n;
		}
		break;
	default:
		vchip_reset(vchip);
	}
}

static int __wcmd(const vchip_slave_t *slave, char cmd)
{
	slave->wb(VCHIP_SOH);
	slave->wb(VCHIP_SOH);
	slave->wb(cmd);
	return 0;
}

static int __wdat(const vchip_slave_t *slave, const void *buf, int n)
{
	const char *p = buf;
	for(int i = 0; i < n; i ++) {
		char byte = p[i];
		if((byte == VCHIP_SOH) || (byte == VCHIP_ESC)) {
			slave->wb(VCHIP_ESC);
		}
		slave->wb(byte);
	}
	return 0;
}

static int __rdat(const vchip_slave_t *slave, void *buf, int n)
{
	char *echo = buf;
	for(int i = 0; i < n; i ++) {
		char byte;
		slave->rb(&byte);
		echo[i] = byte;
	}
	return 0;
}

int vchip_outl(const vchip_slave_t *slave, unsigned addr, unsigned value)
{
	char ecode;
	__wcmd(slave, VCHIP_AR);
	__wdat(slave, &addr, 2);
	__rdat(slave, &ecode, 1);
	if(ecode == ECODE_OK) {
		__wcmd(slave, VCHIP_WL);
		__wdat(slave, &value, 4);
		__rdat(slave, &ecode, 1);
	}
	return (ecode == ECODE_OK) ? 0 : ecode;
}

int vchip_inl(const vchip_slave_t *slave, unsigned addr, unsigned *value)
{
	char ecode;
	__wcmd(slave, VCHIP_AR);
	__wdat(slave, &addr, 2);
	__rdat(slave, &ecode, 1);
	if(ecode == ECODE_OK) {
		__wcmd(slave, VCHIP_RL);
		__rdat(slave, value, 4);
	}
	return (ecode == ECODE_OK) ? 0 : ecode;
}
