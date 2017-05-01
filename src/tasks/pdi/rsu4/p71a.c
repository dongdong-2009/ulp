/*
	feng.ni 2017/04/08 initial version
 */

#include "config.h"
#include "ulp/sys.h"
#include "can.h"
#include "priv/usdt.h"
#include "shell/cmd.h"
#include <string.h>

#include "bsp.h"
#include "pdi.h"
#include "../ecu/ecu.h"

const char *pdi_fixture_id = "R035";

#if 1
//diag ecu is used
int pdi_test(int pos, int mask, pdi_report_t *report)
{
	static can_msg_t ecu_rx_msg;
	#define rxdata ecu_rx_msg.data

	ecu_can_init(0, 0x5a0);
	pdi_can_rx(&ecu_rx_msg);
	pdi_can_rx(&ecu_rx_msg);
	pdi_can_rx(&ecu_rx_msg);
	int ecode = pdi_can_rx(&ecu_rx_msg);
	if(ecode) { //msg not received :(
		return -1;
	}

	int temp = 0;
	temp |= rxdata[0] << 16;
	temp |= rxdata[1] << 8;
	temp |= rxdata[0] << 0;
	temp >>= 7;

	//ECU_NG L1 L2 L3 L4 R1 R2 R3 R4
	debug("PDI: rsu diag result = 0x%04x\n", temp);
	if(temp & (1 << 16)) { //ECU_NG
		return -2;
	}

	//fill the report
	temp >>= (pos == 0) ? 8 : 0;
	report->result.byte = (unsigned char) (temp & 0xff);
	report->datalog[0] = rxdata[0];
	report->datalog[1] = rxdata[1];
	report->datalog[2] = rxdata[2];
	report->datalog[3] = rxdata[3] + 1; //+1??? jiamao.gu do not remember why now
	report->datalog[4] = rxdata[4] + 1;
	report->datalog[5] = rxdata[5] + 1;
	report->datalog[6] = rxdata[6] + 1;
	report->datalog[7] = rxdata[7] + 1;
	return 0;
}

#else
//normal ecu is used
int pdi_test(int pos, int mask, pdi_report_t *report)
{
	ecu_can_init(0x752, 0x772);
}
#endif
