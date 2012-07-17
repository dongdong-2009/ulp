/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_BURN_H_
#define __NEST_BURN_H_

#define BURN_BAUD 500000

//arbitary value
#define BURN_INBOX_ADDR 0x0F000000
#define BURN_OUTBOX_ADDR 0x0F000100

/* fail case:
1), not fire ...
2), too many lost ... very likely burn board failure
3), vp_avg & ip_avg over range ---- igbt issue!!!!
*/

/*!!!!burn data, exchanged with nest awctrl board!!!!*/
struct burn_data_s {
	int vp, vp_avg, vp_min, vp_max; //immediate value, average value, unit:
	int ip, ip_avg, ip_min, ip_max;
	unsigned short tp, crc; //fire period, unit:mS
	unsigned short wp, crc_flag; //fire pulse width, immediate value, unit: nS
	unsigned short fire, lost; //fire&lost count, immediate value
};

struct burn_cfg_s {
	char cmd;
	char option; //'S'
	unsigned short vcal; //G = [1 - 65535] / 4096
	unsigned short ical; //G = [1 - 65535] / 4096
	unsigned short wp; //unit: nS, rational range: 0~65535
	unsigned short tp; //unit: mS
};

enum {
	BURN_CMD_CONFIG = 1, //CMD + 's' + (ushort)vpm_ratio_cal + (ushort)ipm_ratio_cal
	BURN_CMD_READ,
};

enum {
	BURN_CH_COILA,
	BURN_CH_COILB,
	BURN_CH_COILC,
	BURN_CH_COILD,
	BURN_CH_NR,
};

#ifdef CONFIG_NEST_BURN
/*note: you may need to re-init can bus after these function call*/
int burn_init(void);
int burn_mask(int ch);
int burn_config(int ch, unsigned short vpm_ratio, unsigned short ipm_ratio, int save);
int burn_read(int ch, struct burn_data_s *result);
int burn_verify(unsigned short *vp, unsigned short *ip, unsigned char *wp);
#else
static inline int burn_init(void) {return 0;}
static inline int burn_mask(int ch) {return 0;}
static inline int burn_config(int ch, unsigned short vpm_ratio, unsigned short ipm_ratio, int save) {return 0;}
static inline int burn_read(int ch, struct burn_data_s *result) {return 0;}
static inline int burn_verify(unsigned short *vp, unsigned short *ip, unsigned char *wp) {return 0;}
#endif

#endif
