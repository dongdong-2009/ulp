/*
 * 	miaofng@2013-1-3 initial version
 *
 *	'vchip' is target for mcu-to-mcu communication,
 *	which is based on physical bus like spi/can/uart ...
 *
 *	it should be:
 *	1, reliable, no error except physical channel off
 *	2, easy-to-use
 *	3, active. host hangup is strictly prohibited! qucik recover ability
 *
 *	frame detail:
 *	1, format: SOH CMD D0 D1 D2 D3
 *	2, if X(CMD or Dn) = SOH, ESC + X is sent
 *
 */
#ifndef __VCHIP_H_
#define __VCHIP_H_

enum {
	VCHIP_AA, /*+4 bytes absolute address*/
	VCHIP_AR, /*+2 bytes relative address*/
	VCHIP_W,
	VCHIP_R,
	VCHIP_SOH = 0xf0,
	VCHIP_ESC,
};

#define VCHIP_WL ((4 << 4) | VCHIP_W)
#define VCHIP_WW ((2 << 4) | VCHIP_W)
#define VCHIP_WB ((1 << 4) | VCHIP_W)
#define VCHIP_RL ((4 << 4) | VCHIP_R)
#define VCHIP_RW ((2 << 4) | VCHIP_R)
#define VCHIP_RB ((1 << 4) | VCHIP_R)

typedef union {
	struct {
		char typ : 4;
		char len : 4; /*0 - 15 bytes to read or write*/
	};
	char value;
} vchip_cmd_t;

typedef struct {
	vchip_cmd_t cmd;
	char flag_soh : 1; /*SOH byte is received*/
	char flag_cmd : 1; /*CMD byte is received*/
	char flag_esc : 1;
	char flag_abs : 1; /*absolute addr mode*/
	char n : 4; /*dat length*/
	char dat[15];
	char ecode;
	unsigned ofs; /*current access pointer*/

	/*public*/
	char *rxd; /*!!! byte newly received, or NULL if none*/
	char *txd; /*!!! byte to be send, or NULL if none*/
} vchip_t;

void vchip_init(void *ri, const void *ro, const void *az, int n);
void vchip_reset(vchip_t *vchip);
void vchip_update(vchip_t *vchip);

typedef struct {
	int (*wb)(char byte);
	int (*rb)(char *byte);
} vchip_slave_t;

int vchip_outl(const vchip_slave_t *slave, unsigned addr, unsigned value);
int vchip_inl(const vchip_slave_t *slave, unsigned addr, void *value);

#endif /*__VCHIP_H_*/
