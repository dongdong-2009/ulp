/*
*
*  miaofng@2014-5-10   initial version
*
*/

#ifndef __IRC_H__
#define __IRC_H__

#include "config.h"

#define CAN_BAUD 500000

/*lsb 4 bits are used for seq counter,  it will be reset by LE_rxd pulse */
#define CAN_ID_DAT 0x100 //0x100-10f
#define CAN_ID_CMD 0x200 //0x200-20f

#define CAN_ID_CFG 0x300

typedef struct {
	unsigned short rsv; //always 0
	unsigned char slot; // 0-127, or 0xFF indicate all slots
	unsigned char bus; //bit mask, 1 on, 0 off, 8 buses at most
	unsigned line; //bit mask, 1 on, 0 off, 32 lines at most
} irc_cfg_msg_t;

#define IRC_RLY_MS 10 //LE wait ms at most
#define IRC_DMM_MS 100 //DMM finish pulse wait at most

#endif
