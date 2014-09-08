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
#define CAN_ID_DPS 0x400

#define IRC_CAN_MS 5 //can communication timeout setting
#define IRC_RLY_MS 10 //LE wait ms at most
#define IRC_DMM_MS 10000 //DMM finish pulse wait at most
#define IRC_UPD_MS 2 //longest system suspend time
#define IRC_POL_MS 1000 //POLL MS
#define IRC_ECO_MS 5 //echo back max delay

enum {
	IRC_MODE_HVR,	//insulation resistance test
	IRC_MODE_L4R,
	IRC_MODE_W4R,
	IRC_MODE_L2R, IRC_MODE_L2T = IRC_MODE_L2R,

	IRC_MODE_RPB,	//probe mode

	IRC_MODE_RMX,	//matrix self test mode
	IRC_MODE_VHV,	//hv calibration mode
	IRC_MODE_VLV,	//lv calibration mode
	IRC_MODE_IIS,	//is calibration mode

	IRC_MODE_OFF,
	IRC_MODE_DEF = IRC_MODE_L2T,
};

void irc_init(void);
void irc_update(void);

#endif
