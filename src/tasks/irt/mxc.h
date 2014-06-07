/*
*
*  miaofng@2014-6-5   initial version
*
*/

#ifndef __MXC_H__
#define __MXC_H__

#include "config.h"


enum {
	MXC_CMD_CFG,
};

typedef struct {
	unsigned char cmd;
	unsigned char ms; //0 -> slot card should dead wait LE signal

	#define MXC_ALL_SLOT	0xff
	#define MXC_ALL_EBUS	0xff
	#define MXC_ALL_IBUS	0x00
	#define MXC_ALL_ELNE	0xffffffff
	#define MXC_ALL_ILNE	0x00000000

	unsigned char slot; // 0-127, or 0xFF indicate all slots
	unsigned char bus; //bit mask, 0 ibus, 1 ebus, 8 buses at most
	unsigned line; //bit mask, 1 external, 0 internal, 32 lines at most
} mxc_cfg_msg_t;

int mxc_init(void);
int mxc_send(const can_msg_t *msg);
int mxc_latch(void);
int mxc_mode(int mode);

#endif
