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

/*in case of any error occurs, main routine will
be trapped until "*CLS" cmd is received
*/
static struct {
	unsigned error; //if set, system enter into error state
} irc_status;

int irc_error_get(void)
{
	return irc_ecode;
}

void irc_error_push(int ecode, const char *file, int line)
{
	irc_ecode = ecode;
	irc_efile = file;
	irc_eline = line;
}

void irc_error_pop(void)
{
	irc_ecode = 0;
	irc_efile = NULL;
	irc_eline = 0;
}

void irc_error_set(int ecode, const char *file, int line)
{
	/*!!!avoid re-entrant!!!*/
	if(ecode && !irc_ecode) {
		ecode = (ecode > 0) ? -ecode : ecode;
		irc_error_push(ecode, file, line);

		irc_status.error = 1;
		led_error(ecode);
		while(irc_status.error) {
			irc_update();
		}
	}
}

void irc_error_clear(void)
{
	irc_error_pop();
	irc_status.error = 0;
	led_flash(LED_GREEN);
}

void irc_error_pop_print_history(void)
{
	_irc_error_print(irc_ecode, irc_efile, irc_eline);
	irc_error_pop();
}

void _irc_error_print(int ecode, const char *file, int line)
{
	const char *msg = NULL;
	switch(-ecode) {
	case IRT_E_OK:
		msg = "No Error";
		break;
	case IRT_E_HV_UP:
		msg = "HV Power-up Fail";
		break;
	case IRT_E_HV_DN:
		msg = "HV Power-down Fail";
		break;
	case IRT_E_HV:
		msg = "DPS HV Voltage Abnormal";
		break;
	case IRT_E_LV:
		msg = "DPS LV Voltage Abnormal";
		break;
	case IRT_E_HS:
		msg = "DPS HS Voltage Abnormal";
		break;
	case IRT_E_CMD_FORMAT:
		msg = "Command Format Error";
		break;
	case IRT_E_CMD_PARA:
		msg = "Command Para Incorrect";
		break;
	case IRT_E_VM_OPQ_FULL:
		msg = "Operation Queue Is Full";
		break;
	case IRT_E_MEM_OUT_OF_USE:
		msg = "Memory is Out of Usage";
		break;
	case IRT_E_OP_REFUSED:
		msg = "Operation is refused";
		break;
	case IRT_E_CAN_SEND:
		msg = "CAN Message Send Fail";
		break;
	case IRT_E_SLOT_NA_OR_LOST:
		msg = "SLOT Timeout Or Not Available";
		break;
	case IRT_E_LATCH_H:
		msg = "SLOT wait LE high Timeout";
		break;
	case IRT_E_LATCH_L:
		msg = "SLOT wait LE low Timeout";
		break;
	case IRT_E_OPCODE:
		msg = "SLOT card opcode recognize Fail";
		break;
	case IRT_E_DMM:
		msg = "DMM handshake Fail";
		break;
	case IRT_E_SLOT:
		msg = "Some slots Fail or Not Response";
		break;
	case IRT_E_OP_REFUSED_DUETO_ESYS:
		msg = "Operation Refused Due To System Has Error";
		break;
	default:
		msg = "Undefined Error";
		break;
	}

	if(file == NULL) {
		if(msg == NULL) {
			printf("<%+d\r\n", ecode);
		}
		else {
			printf("<%+d,\"%s\"\r\n", ecode, msg);
		}
	}
	else {
		msg = (msg == NULL) ? "" : msg;
		printf("<%+d,\"%s(%s:%d)\"\r\n", ecode, msg, strrchr(file, '\\')+1, line);
	}
}
