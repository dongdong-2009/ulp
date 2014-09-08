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

static int irc_slots = 0;
static time_t irc_poll_timer = 0;
static int irc_poll_slot = 0;

static int irc_is_opc(void)
{
	int opc = vm_is_opc();
	return !opc;
}

static void irc_abort(void)
{
	vm_abort();
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

void irc_poll(void)
{
	if(time_left(irc_poll_timer) < 0) {
		irc_poll_timer = time_get(IRC_POL_MS);
		mxc_ping(irc_poll_slot);
		irc_poll_slot = (irc_poll_slot < irc_slots) ? (irc_poll_slot + 1) : 0;
	}
}

/*implement a group of switch operation*/
void irc_update(void)
{
	sys_update();
	irc_poll();
}

int main(void)
{
	sys_init();
	led_flash(LED_RED);
	printf("irc v1.1, SW: %s %s\n\r", __DATE__, __TIME__);
	irc_init();
	vm_init();
	lv_config(DPS_KEY_U, 0.0);
	irc_mode(IRC_MODE_L2T);
	while(1) {
		irc_update();
		vm_update();
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
		"ABORT		abort current fail operation\n"
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
		_irc_error_print(irc_error_get(), NULL, 0);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		board_reset();
	}
	else if(!strcmp(argv[0], "ABORT")) {
		irc_abort();
		printf("<%+d\n\r", 0);
		return 0;
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		ecode = -IRT_E_CMD_FORMAT;
	}

	irc_error_print(ecode);
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
	irc_error_print(ecode);
	return 0;
}
const cmd_t cmd_mode = {"MODE", cmd_mode_func, "change irc work mode"};
DECLARE_SHELL_CMD(cmd_mode)

static int cmd_power_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"POWER LV <value>	change lv voltage\n"
	};

	int ecode = -IRT_E_CMD_FORMAT;
	if((argc > 1) && !strcmp(argv[1], "HELP")) {
		printf("%s", usage);
		return 0;
	}
	else if((argc == 3) && !strcmp(argv[1], "LV")) {
		float v = atof(argv[2]);
		ecode = - IRT_E_CMD_PARA;
		if((v >= 0.0) && (v <= 30.0)) {
			ecode = lv_config(DPS_KEY_U, v);
		}
	}

	irc_error_print(ecode);
	return 0;
}
const cmd_t cmd_power = {"POWER", cmd_power_func, "power board ctrl"};
DECLARE_SHELL_CMD(cmd_power)
