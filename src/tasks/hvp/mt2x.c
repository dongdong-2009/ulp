/*
 * 	miaofng@2010 initial version
 *		it contains the program algo for mt2x ecu such as mt20u/20u2/22u
 */

#include "config.h"
#include "mt2x.h"
#include "hvp.h"
#include "util.h"
#include "common/kwp.h"
#include "common/print.h"
#include "common/inifile.h"
#include <stdio.h>
#include <stdlib.h>

#include "ff.h"

#define __DEBUG

int mt2x_Init(void)
{
	char ptp_util[32];
	char ptp[32];
	
	/*read config from kwp.ini*/
	read_profile_string("files", "ptp_util", ptp_util, 31, "ptp_util", "kwp.ini");
	read_profile_string("files", "ptp", ptp, 31, "ptp", "kwp.ini");
	
#ifdef __DEBUG
	print("util file: %s\n", ptp_util);
	print("ptp  file: %s\n", ptp);
#endif

	util_init(ptp_util, ptp);
	return 0;
}

static int mt2x_dnld(int addr, int size)
{
	int btr, br;
	char buf[32];
	
	btr = 32;
	while(1) {
		util_read(buf, btr, &br);
		if(br <= 0)
			break;
		
		//download
		kwp_TransferData(addr, br, buf);
		addr += br;
	}
	
	return 0;
}

int mt2x_Prog(void)
{
	int addr, size;
	addr = util_addr();
	size = util_size();
	
	kwp_Init();
	kwp_EstablishComm();
	kwp_StartDiag(0x85, 0x00);
	kwp_RequestToDnload(0, addr, size, 0);
	mt2x_dnld(addr, size);
	kwp_RequestTransferExit();
	kwp_StartRoutineByAddr(addr);
	util_interpret();
	util_close();
	return 0;
}
