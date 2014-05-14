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

enum {
	IRC_CFG_NORMAL, /*config is determined by para slot&bus&line*/
	IRC_CFG_ALLOFF_IBUS_ELNE, /*for normal test reset use*/
	IRC_CFG_ALLOFF_EBUS_ILNE, /*for matrix self test use*/
	IRC_CFG_ALLOFF_EBUS_ELNE, /*for probe mode use*/
};

typedef struct {
	unsigned char cmd;
	unsigned char rsv; //always 0
	unsigned char slot; // 0-127, or 0xFF indicate all slots
	unsigned char bus; //bit mask, 0 ibus, 1 ebus, 8 buses at most
	unsigned line; //bit mask, 1 external, 0 internal, 32 lines at most
} irc_cfg_msg_t;

#define IRC_RLY_MS 10 //LE wait ms at most
#define IRC_DMM_MS 10000 //DMM finish pulse wait at most

#define IRC_MASK_HV	(1<<1)
#define IRC_MASK_IS	(1<<5) //1, current source on, to measure resistor
#define IRC_MASK_LV	(1<<7) //1, low voltage source on, to power uut relay
#define IRC_MASK_EBUS	(1<<8) //1, bus switch = external
#define IRC_MASK_IBUS	(0<<8) //1, bus switch = internal
#define IRC_MASK_ELNE	(1<<9) //1, line switch = external
#define IRC_MASK_ILNE	(0<<8) //1, line switch = internal
#define IRC_MASK_EIS	(1<<10) //1, IS could be turn on
#define IRC_MASK_ELV	(1<<11) //1, LV could be turn on
#define IRC_MASK_ADD	(1<<11) //1, additional matrix switch operation is need

enum {
	IRC_MODE_HVR = 0x7f | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ELV,
	IRC_MODE_L4R = 0x48 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_EIS,
	IRC_MODE_W4R = 0x08 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ELV | IRC_MASK_EIS,
	IRC_MODE_L2R = 0x00 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ELV,
	IRC_MODE_L2T = IRC_MODE_L2R,
	IRC_MODE_RMX = 0x1c | IRC_MASK_IBUS | IRC_MASK_ILNE,
	IRC_MODE_VHV = 0x02 | IRC_MASK_EBUS | IRC_MASK_ILNE,
	IRC_MODE_VLV = 0x80 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ADD, //!!!ISSUE!!!
	IRC_MODE_IIS = 0x21 | IRC_MASK_IBUS | IRC_MASK_ELNE,
};

#endif
