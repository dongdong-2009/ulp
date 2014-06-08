/*
*
*  miaofng@2014-5-10   initial version
*
*/

#include "ulp/sys.h"
#include "irc.h"
#include "vm.h"
#include "nvm.h"
#include "led.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"
#include "can.h"
#include "err.h"
#include "led.h"
#include "bsp.h"
#include "mxc.h"

static int irc_ecode = 0;
static int irc_slots = 0;
static struct {
	unsigned opc : 1;
	unsigned rst : 1;
	unsigned abt : 1;
} irc_status;

int dmm_trig(void)
{
	int ecode = -IRT_E_DMM;
	time_t deadline = time_get(IRC_DMM_MS);
	time_t suspend = time_get(IRC_UPD_MS);

	trig_set(1);
	while(time_left(deadline) > 0) {
		if(trig_get() == 1) { //vmcomp pulsed
			trig_set(0);
			ecode = 0;
			break;
		}

		if(time_left(suspend) < 0) {
			sys_update();
		}
	}

	return ecode;
}

static int irc_is_opc(void)
{
	int opc = vm_is_opc();
	opc = (irc_status.opc) ? opc : 0;
	return opc;
}

static int irc_abort(void)
{
	vm_init();
	return 0;
}

static int irc_mode(int mode)
{
	rly_set(IRC_MODE_OFF);
	int ecode = mxc_mode(mode);
	rly_set(mode);
	return ecode;
}

void irc_init(void)
{
	board_init();
	mxc_init();
	trig_set(0);
	irc_slots = mxc_scan(0, 0);
}

/*implement a group of switch operation*/
void irc_update(void)
{
	int cnt = 0, bytes, over;
	can_msg_t msg;

	if(irc_ecode) {
		return;
	}

	do {
		bytes = 8;
		irc_ecode = vm_fetch(msg.data, &bytes);
		over = vm_opgrp_is_over();
		if(!irc_ecode && bytes > 0) {
			//send can message
			msg.dlc = (char) bytes;
			msg.id =  over ? CAN_ID_CMD : CAN_ID_DAT;
			msg.id += cnt & 0x0F;
			irc_ecode = mxc_send(&msg);
			if(irc_ecode) {
				return;
			}

			cnt ++;

			//LE operation is needed?
			if(over) {
				irc_ecode = mxc_latch();
				if(irc_ecode) { //find the bad guys hide inside the good slots
					return;
				}

				if(vm_opgrp_is_scan()) {
					irc_ecode = dmm_trig();
					if(irc_ecode) {
						return;
					}
				}
			}
		}

		if(irc_status.abt) {
			irc_abort();
			return;
		}
	} while(!over);
}

int main(void)
{
	sys_init();
	led_flash(LED_RED);
	printf("irc v1.1, SW: %s %s\n\r", __DATE__, __TIME__);
	irc_init();
	vm_init();
	irc_mode(IRC_MODE_L2T);
	while(1) {
		sys_update();
		irc_status.opc = 0;
		irc_update();
		irc_status.opc = 1;
	}
}

#include "shell/cmd.h"

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*OPC?		operation is completed?\n"
		"*ERR?		error code & info\n"
		"*RST		instrument reset\n"
		"ABORT		abort operation queue left\n"
	};

	int ecode = 0;
	if(!strcmp(argv[0], "*IDN?")) {
		printf("<Linktron Technology,IRT16X3254,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*OPC?")) {
		int opc = irc_is_opc();
		printf("<%+d\n\r", opc);
		return 0;
	}
	else if(!strcmp(argv[0], "*ERR?")) {
		err_print(irc_ecode);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		board_reset();
	}
	else if(!strcmp(argv[0], "ABORT")) {
		irc_status.abt = 1;
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		ecode = -IRT_E_CMD_FORMAT;
	}

	err_print(ecode);
	return 0;
}

static int cmd_mode_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"MODE <mode>\n"
		"	<mode> = [HVR|L4R|W4R|L2R|L2T|PRB|RMX|VHV|VLV|IIS]\n"
	};

	int ecode = 0;
	int lv = (argc > 2) ? atoi(argv[2]) : 0;
	int is = (argc > 3) ? atoi(argv[3]) : 0;
	const char *name[] = {
		"HVR", "L4R", "W4R", "L2R", "L2T", "RPB",
		"RMX", "VHV", "VLV", "IIS", "OFF", "DEF"
	};
	const int mode_list[] = {
		IRC_MODE_HVR, IRC_MODE_L4R,
		IRC_MODE_W4R, IRC_MODE_L2R,
		IRC_MODE_L2T, IRC_MODE_RPB,

		IRC_MODE_RMX, IRC_MODE_VHV,
		IRC_MODE_VLV, IRC_MODE_IIS,
		IRC_MODE_OFF, IRC_MODE_DEF,
	};

	if(!strcmp(argv[1], "HELP")) {
		printf("%s", usage);
		return 0;
	}

	ecode = -IRT_E_CMD_FORMAT;
	for(int i = 0; i < sizeof(mode_list) / sizeof(int); i ++) {
		if(!strcmp(argv[1], name[i])) {
			int mode = mode_list[i];
			ecode = -IRT_E_OP_REFUSED;
			if(irc_is_opc()) {
				ecode = irc_mode(mode);
				break;
			}
		}
	}
	err_print(ecode);
	return 0;
}
const cmd_t cmd_mode = {"MODE", cmd_mode_func, "change irc work mode"};
DECLARE_SHELL_CMD(cmd_mode)
