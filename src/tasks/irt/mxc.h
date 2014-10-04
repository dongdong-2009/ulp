/*
*  design hints: (dead wait a response msg is not allowed!!!)
*  1, mxc send can msg INT to irc once 100mS if it's in INIT(power-up) state
*  2, irc response with OFFLINE then adds the slot to active list
*  3, when pc issue MODE cmd to irc, irc bring all slots to ready state
*  4, irc poll slot cards periodly and remove the dead card from the active list(error?yes)
*  5, if irc encounter an error, slot hold le signal then send INT msg to irc once 100mS until error cleared
*
*  miaofng@2014-6-5   initial version
*  miaofng@2014-9-17 mxc communication protocol update(fast birth slow die:)
*
*/

#ifndef __MXC_H__
#define __MXC_H__

#include "config.h"
#include "can.h"
#include "linux/list.h"

/*note: to avoid conflict, each node shouldn't send the same can id, so
node 0=> irc board, node 1=> slot 0, ...*/
#define MXC_NODE(slot) (slot + 1)
#define MXC_SLOT(node) (node - 1)
#define MXC_NODE_ALL (0)
#define MXC_SLOT_ALL (-1)

/*
#define MXC_ALL_SLOT	0xff
#define MXC_ALL_EBUS	0xff
#define MXC_ALL_IBUS	0x00
#define MXC_ALL_ELNE	0xffffffff
#define MXC_ALL_ILNE	0x00000000
*/

enum {
	MXC_CMD_RESET,
	MXC_CMD_MODE,
	MXC_CMD_PING,
	MXC_CMD_OFFLINE,
};

typedef struct {
	unsigned char cmd;
	unsigned char ms : 7; //0 -> slot card should dead wait LE signal
	unsigned char safelatch : 1;
	unsigned char mode;
	unsigned char vbus_sw; //bit mask,  1->relay on, debug mode only
	unsigned line_sw; //bit mask, 1->relay on, debug mode only
} mxc_cfg_t;

typedef struct {
	int ecode;
	int flag;
} mxc_echo_t;

/*SLOT TYPE*/
enum {
	MXC_ALL,
	MXC_GOOD, /*good slot, ecode = 0*/
	MXC_FAIL, /*fail slot*/
	MXC_SELF, /*slot has self diagnosis function*/
	MXC_DCFM, /*slot has DIN41612 calibration fixture mounted*/
	MXC_INIT,
	MXC_OFFLINE,
};

#if CONFIG_IRT_IRC
struct mxc_s {
	int slot;
	int flag; // (1 << MXC_SELF )? | (1 << MXC_DCFM)?
	int ecode;
	int nlost; //lost counter
	time_t timer;
	struct list_head list;
};

void mxc_can_handler(can_msg_t *msg);

void mxc_init(void);
void mxc_update(void);
int mxc_latch(void);
int mxc_mode(int mode);

struct mxc_s *mxc_search(int slot);
int mxc_scan(void *image, int type); //return nr of working slots&slot list
int mxc_reset(int slot);
int mxc_ping(int slot, int ms); //if(ms > 0) return current(or last) ecode of the slot
int mxc_offline(int slot);
#endif

#endif
