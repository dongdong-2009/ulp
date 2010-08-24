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
#include "time.h"
#include "uart.h"

#include "ff.h"

#define __DEBUG

int mt2x_Init(void)
{
	char ptp_util[32];
	char ptp[32];
	
	/*read config from kwp.ini*/
	read_profile_string("files", "ptp_util", ptp_util, 31, "util.ptp", "hvp.ini");
	read_profile_string("files", "ptp", ptp, 31, "cal.ptp", "hvp.ini");
	
#ifdef __DEBUG
	print("util file: %s\n", ptp_util);
	print("ptp  file: %s\n", ptp);
#endif

	return util_init(ptp_util, ptp);
}

int mt2x_Prog(void)
{
	int err;
	kwp_Init(& uart1);
	err = util_interpret();
	util_close();
	return err;
}
