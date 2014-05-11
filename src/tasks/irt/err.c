/*
*
*  miaofng@2014-5-11   initial version
*
*/

#include "ulp/sys.h"
#include "vm.h"
#include <string.h>
#include "err.h"

void err_print(int ecode)
{
	const char *msg = NULL;
	switch(-ecode) {
	case IRT_E_OK:
		msg = "No error";
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
		msg = "Operation is not allowed";
		break;
	default:
		msg = "Undefined Error";
		break;
	}
	printf("<%+d,\"%s\"\n\r", ecode, msg);
}
