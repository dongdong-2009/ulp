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

static int mt2x_addr;

int mt2x_Init(void)
{
	char ptp_util[32];
	char ptp[32];
	char addr[16];
	
	/*read config from kwp.ini*/
	read_profile_string("files", "ptp_util", ptp_util, 31, "ptp_util", "hvp.ini");
	read_profile_string("files", "ptp", ptp, 31, "ptp", "hvp.ini");
	read_profile_string("info", "addr_util", addr, 15, "0x003800", "hvp.ini");
	
#ifdef __DEBUG
	print("util file: %s\n", ptp_util);
	print("ptp  file: %s\n", ptp);
#endif

	sscanf(addr, "0x%x", &mt2x_addr);
	return util_init(ptp_util, ptp);
}

static int mt2x_dnld(int addr, int size)
{
	int btr, br, try;
	char buf[32];
	
	btr = 32;
	while(1) {
		util_read(buf, btr, &br);
		if(br <= 0)
			break;
		
		//download
		try = 0;
		while(kwp_TransferData(addr, br, buf)) {
#ifdef __DEBUG
			print("Resend %d times\n", try);
#endif
			try ++;
			if(try > 2) {
				return -1;
			}
		}
		
		addr += br;
	}
	
	return 0;
}

int mt2x_Prog(void)
{
	int addr, size;
	int err;
	
	//addr = util_addr();
	addr = mt2x_addr;
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
	util_interpret();
	util_close();
	return err;
}
