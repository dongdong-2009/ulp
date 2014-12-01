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
#include "led.h"

static int irc_ecode = 0;
static const char *irc_efile = NULL;
static int irc_eline;

#define DEBUG_ERR 0

int irc_error_get(void)
{
	return irc_ecode;
}

int irc_error_set(int ecode, const char *file, int line)
{
	ecode = (ecode > 0) ? -ecode : ecode;
	if(ecode) {
		if(irc_ecode == IRT_E_OK) {
			led_error(ecode);
			irc_ecode = ecode;
			irc_efile = file;
			irc_eline = line;
#if DEBUG_ERR
			_irc_error_print(ecode, irc_efile, irc_eline);

			while(irc_ecode) {
				irc_update();
			}
#endif
		}
	}
	return irc_ecode;
}

int irc_error_clear(void)
{
	led_flash(LED_GREEN);
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
	case IRT_E_HV_UP:
		msg = "HV Power-up Fail";
		break;
	case IRT_E_HV_DN:
		msg = "HV Power-down Fail";
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
	case IRT_E_SLOT_LOST:
		msg = "SLOT Response Timeout";
		break;
	case IRT_E_SLOT_LATCH_H:
		msg = "SLOT wait LE high Timeout";
		break;
	case IRT_E_SLOT_LATCH_L:
		msg = "SLOT wait LE low Timeout";
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
	case IRT_E_SLOT_SCAN_FAIL:
		msg = "Some slots Fail or Not Response";
		break;
	case IRT_E_OP_REFUSED_ESYS:
		msg = "Operation Refused Due To System Has Error";
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
