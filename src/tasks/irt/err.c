/*
*
*  miaofng@2014-5-11   initial version
*
*/

#include "ulp/sys.h"
#include "vm.h"
#include <string.h>
#include "err.h"
#include "irc.h"

static int irc_ecode = 0;
static const char *irc_efile = NULL;
static int irc_eline;

int irc_error_get(void)
{
	return irc_ecode;
}

int irc_error_set(int ecode, const char *file, int line)
{
	ecode = (ecode > 0) ? -ecode : ecode;
	if(ecode) {
		irc_ecode = ecode;
		irc_efile = file;
		irc_eline = line;

		_irc_error_print(ecode, irc_efile, irc_eline);

		while(irc_ecode) {
			irc_update();
		}
	}
	return irc_ecode;
}

int irc_error_clear(void)
{
	irc_ecode = 0;
	irc_efile = NULL;
	irc_eline = 0;
	return irc_ecode;
}

void _irc_error_print(int ecode, const char *file, int line)
{
	if(file == NULL) {
		ecode = irc_ecode;
		file = irc_efile;
		line = irc_eline;
	}

	const char *msg = NULL;
	switch(-ecode) {
	case IRT_E_OK:
		msg = "No error";
		break;
	case IRT_E_NA:
		msg = "Not Available";
		break;
	case IRT_E_CMD_FORMAT:
		msg = "Command Format Error";
		break;
	case IRT_E_CMD_PARA:
		msg = "Command Para Incorrect";
		break;
	case IRT_E_OPQ_FULL:
		msg = "Operation Queue Is Full";
		break;
	case IRT_E_MEM_OUT_OF_USE:
		msg = "Memory is Out of Usage";
		break;
	case IRT_E_OP_REFUSED:
		msg = "Operation is refused";
		break;
	case IRT_E_CAN:
		msg = "CAN communication Fail";
		break;
	case IRT_E_SLOT:
		msg = "SLOT handshake Fail";
		break;
	case IRT_E_SLOT_LATCH:
		msg = "SLOT card LE latch Fail";
		break;
	case IRT_E_SLOT_RESTORE:
		msg = "SLOT card LE restore Fail";
		break;
	case IRT_E_SLOT_OPCODE:
		msg = "SLOT card opcode recognize Fail";
		break;
	case IRT_E_DMM:
		msg = "DMM handshake Fail";
		break;
	case IRT_E_VM:
		msg = "VM runtime Error";
		break;
	default:
		msg = "Undefined Error";
		break;
	}

	if(ecode && (file != NULL)) {
		printf("<%+d,\"%s(%s:%d)\"\r\n", ecode, msg, strrchr(file, '\\')+1, line);
	}
	else {
		printf("<%+d,\"%s\"\r\n", ecode, msg);
	}
}
