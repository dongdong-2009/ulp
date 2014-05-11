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
	IRT_E_CMD_FORMAT,
	IRT_E_CMD_PARA,
	IRT_E_OPQ_FULL,
	IRT_E_OPQ_DATA_ERROR,
	IRT_E_MEM_OUT_OF_USE,
	IRT_E_OP_REFUSED,
};

void err_print(int ecode);
#endif
