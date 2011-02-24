/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "cncb.h"
#include "nest_can.h"

int nest_can_sel(int ch)
{
	if(ch == DW_CAN) {
		cncb_signal(CAN_SEL, SIG_HI);
		return 0;
	}

	if(ch == SW_CAN) { //md0, md1 00->sleep, 01->wake, 10->tx high speed, 11 -> normal speed&voltage
		cncb_signal(CAN_SEL, SIG_LO);
		cncb_signal(CAN_MD0, SIG_HI);
		cncb_signal(CAN_MD1, SIG_HI);
		return 0;
	}

	return -1;
}
