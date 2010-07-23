/*
 * 	miaofng@2010 initial version
 *		it contains the program algo for mt2x ecu such as mt20u/20u2/22u
 */

#include "config.h"
#include "mt2x.h"
#include "hvp.h"
#include "util.h"
#include "common/kwp.h"
#include <stdio.h>
#include <stdlib.h>

static char mt2x_stm;
static mt2x_model_t mt2x_model;

/*read config from config file*/
int mt2x_Init(void)
{
	mt2x_stm = START;

	kwp_Init();
	util_Init();
	return 0;
}

int mt2x_Update(void)
{
	int ret = mt2x_stm;
	int repeat = 0;

	if(!kwp_IsReady())
		return ret;

	switch(mt2x_stm) {
	case START:
		repeat = kwp_EstablishComm();
		break;
	case START_COMM:
		repeat = kwp_StartComm(0, 0);
		break;
	case START_DIAG:
		repeat = kwp_StartDiag(0x85, 0x00);
		break;
	case REQ_DNLOAD:
		repeat = kwp_RequestToDnload(0x00, mt2x_model.addr, mt2x_model.size, 0);
		break;
	case DNLOAD:
		repeat = util_Dnload();
		break;
	case DNLOAD_EXIT:
		repeat = kwp_RequestTransferExit();
		break;
	case EXECUTE:
		repeat = kwp_StartRoutineByAddr(mt2x_model.addr);
		break;
	case INTERPRETER:
		repeat = util_Update();
		break;
	default:
		mt2x_stm = READY;
	}

	if(!repeat)
		mt2x_stm ++;

	if(repeat < 0) {
		//error handling
	}

	return ret;
}

const pa_t mt2x = {
	.name = "mt2x",
	.init = mt2x_Init,
	.update = mt2x_Update,
};
