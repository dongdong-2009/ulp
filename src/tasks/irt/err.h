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
	IRT_E_NA, /*NOT AVAILABLE*/
	IRT_E_CAN,
	IRT_E_SLOT,
	IRT_E_SLOT_LATCH,
	IRT_E_SLOT_RESTORE,
	IRT_E_SLOT_OPCODE,
	IRT_E_CMD_FORMAT, /*command syntax error*/
	IRT_E_CMD_PARA, /*command syntax OK, but is illegal*/
	IRT_E_OPQ_FULL,
	IRT_E_OPQ_DATA_ERROR,
	IRT_E_MEM_OUT_OF_USE,
	IRT_E_OP_REFUSED,
	IRT_E_DMM,
	IRT_E_VM, /*vm execute error*/
};

#define irc_error(ecode) do { \
	irc_error_set(ecode, __BASE_FILE__, __LINE__); \
} while(0)

#define irc_error_print(ecode) do { \
	_irc_error_print(ecode, __BASE_FILE__, __LINE__); \
} while(0)

int irc_error_get(void);
int irc_error_set(int ecode, const char *file, int line);
int irc_error_clear(void);
void _irc_error_print(int ecode, const char *file, int line);
#endif
