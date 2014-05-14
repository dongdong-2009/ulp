/*
*
*  miaofng@2014-5-10   initial version
*
*/

#ifndef __IRT_ERR_H__
#define __IRT_ERR_H__

#include "config.h"

enum {
	IRT_E_OK,
	IRT_E_CMD_FORMAT, /*command syntax error*/
	IRT_E_CMD_PARA, /*command syntax OK, but is illegal*/
	IRT_E_OPQ_FULL,
	IRT_E_OPQ_DATA_ERROR,
	IRT_E_MEM_OUT_OF_USE,
	IRT_E_OP_REFUSED,
	IRT_E_CAN,
	IRT_E_SLOT,
	IRT_E_DMM,
	IRT_E_VM, /*vm execute error*/
};

void err_print(int ecode);
#endif
