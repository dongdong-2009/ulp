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

const char *pdi_fixture_id = "R512";

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

	/*
	NOK	5A0 2a aa 80 00 00 00 00 00    0010 1010 1010 1010
	1OK	5A0 0a aa 80 00 00 00 00 00    0000 1010 1010 1010
	2OK	5A0 22 aa 80 00 00 00 00 00    0010 0010 1010 1010
	3OK	5A0 28 aa 80 00 00 00 00 00    0010 1000 1010 1010
	4OK	5A0 2a 2a 80 00 00 00 00 00    0010 1010 0010 1010
	*/

	int temp = 0;
	temp |= rxdata[0] << 8;
	temp |= rxdata[1] << 0;
	temp >>= 6;

	//ECU_NG 1 2 3 4
	debug("PDI: rsu diag result = 0x%04x\n", temp);
	if(temp & (1 << 16)) { //ECU_NG
		return -2;
	}

	//fill the report
	report->result.byte = (unsigned char) (temp & 0xff);
	report->datalog[0] = rxdata[0];
	report->datalog[1] = rxdata[1];
	report->datalog[2] = rxdata[2];
	report->datalog[3] = rxdata[3];
	report->datalog[4] = rxdata[4];
	report->datalog[5] = rxdata[5];
	report->datalog[6] = rxdata[6];
	report->datalog[7] = rxdata[7];
	return 0;
}
