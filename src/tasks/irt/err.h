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
	IRT_E_DMM,
	IRT_E_CAN_SEND,
	IRT_E_SLOT, //slot lost or has error
	IRT_E_LATCH_H, //normally held by some fail slots
	IRT_E_LATCH_L,
	IRT_E_OPCODE, //vm or slot runtime error
	IRT_E_HV_UP,
	IRT_E_HV_DN,
	IRT_E_HV,
	IRT_E_LV,
	IRT_E_HS,
	IRT_E_MEM_OUT_OF_USE,

	//host cmd return value
	IRT_E_SLOT_NA_OR_LOST,
	IRT_E_CMD_FORMAT, /*command syntax error*/
	IRT_E_CMD_PARA, /*command syntax OK, but is illegal*/
	IRT_E_VM_OPQ_FULL,
	IRT_E_VM_OPQ_DATA_ERROR,
	IRT_E_OP_REFUSED,
	IRT_E_OP_REFUSED_DUETO_ESYS,
};

#define irc_error(ecode) do { \
	irc_error_set(ecode, __BASE_FILE__, __LINE__); \
} while(0)

#define irc_error_print(ecode) do { \
	_irc_error_print(ecode, NULL, 0); \
} while(0)

#define irc_error_print_long(ecode) do { \
	_irc_error_print(ecode, __BASE_FILE__, __LINE__); \
} while(0)

int irc_error_get(void);
void irc_error_clear(void);
void irc_error_pop_print_history(void);

//private
void irc_error_set(int ecode, const char *file, int line);
void _irc_error_print(int ecode, const char *file, int line);
#endif
