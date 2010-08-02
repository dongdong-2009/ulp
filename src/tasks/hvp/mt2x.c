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
	read_profile_string("files", "ptp_util", ptp_util, 31, "ptp_util", "hvp.ini");
	read_profile_string("files", "ptp", ptp, 31, "ptp", "hvp.ini");
	
#ifdef __DEBUG
	print("util file: %s\n", ptp_util);
	print("ptp  file: %s\n", ptp);
#endif

	return util_init(ptp_util, ptp);
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
	int err;
	
	addr = util_addr();
	size = util_size();

#ifdef __DEBUG
	print("util addr = 0x%06x\n", addr);
	print("util size = 0x%06x\n", size);
#endif
	
	err = kwp_Init();
	if(!err) err = kwp_EstablishComm();
	if(!err) err =  kwp_StartDiag(0x85, 0x00);
	if(!err) {
		err =  kwp_RequestToDnload(0, addr, size, 0);
		if(!err) {
			err = mt2x_dnld(addr, size);
			err += kwp_RequestTransferExit();
		}
		if(!err) err =  kwp_StartRoutineByAddr(addr);
		if(!err) err =  util_interpret();
	}
	
	util_close();
	return err;
}
