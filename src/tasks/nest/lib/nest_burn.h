/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_BURN_H_
#define __NEST_BURN_H_

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
	unsigned short tp, tp_set; //fire period, unit:mS, tp_set -> burn_ms
	unsigned short wp, wp_set; //fire pulse width, immediate value, unit: 1/36 us, wp_set -> mos_delay_clks
	unsigned short fire, lost; //fire&lost count, immediate value
};

enum {
	BURN_CMD_CONFIG = 1,
	BURN_CMD_READ,
};

enum {
	BURN_CH_COILA,
	BURN_CH_COILB,
	BURN_CH_COILC,
	BURN_CH_COILD,
};

int burn_config(int ch, const struct burn_data_s *config);
int burn_read(int ch, struct burn_data_s *result);

#endif
