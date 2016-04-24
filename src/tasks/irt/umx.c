/*
*
*  miaofng@2015-8-25   initial version
*
*/

#include "ulp/sys.h"
#include "vm.h"
#include "nvm.h"
#include "led.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"
#include "err.h"
#include "led.h"
#include "umx_bsp.h"

//max wait time for VMCOMP pulse
#define UMX_DMM_MS 10000

static int umx_busy = 0;

static void umx_init(void);
static void umx_update(void);

/*trig dmm to do measurement and wait for operation finish*/
static int umx_measure(void)
{
	int ecode = -IRT_E_DMM;
	time_t deadline = time_get(UMX_DMM_MS);

	trig_set(1);
	while(time_left(deadline) > 0) {
		umx_update();
		if(trig_get() == 1) { //vmcomp pulsed
			trig_set(0);
			ecode = 0;
			break;
		}
	}

	irc_error(ecode);
	return ecode;
}

void vm_execute(opcode_t opcode, opcode_t seq)
{
	static int scan = 0;
	static int opnr = 0;
	int type = opcode.type;
	umx_busy = 1;

	if(type == VM_OPCODE_GRUP) {
		int bypass = VM_SCAN_OVER_GROUP && scan && vm_get_scan_cnt();
		if(opnr > 0 && !bypass) {
			opnr = 0;
			mxc_execute();
			if(!scan) {
				mxc_image_store();
			}
			else {
				vm_wait(vm_get_measure_delay());
				umx_measure();

				//restore to static relay status
				mxc_image_restore();
				mxc_execute();
				vm_wait(vm_get_measure_delay());
			}
		}
		return;
	}

	/*save current opcode type for later use*/
	type = (type == VM_OPCODE_SEQU) ? seq.type : type;
	scan = (type == VM_OPCODE_SCAN) || (type == VM_OPCODE_FSCN);

	opcode_t target = (type == VM_OPCODE_SEQU) ? seq : opcode;
	if(type == VM_OPCODE_FSCN) {
		int min = opcode.fscn.fscn;
		int max = target.fscn.fscn;

		for(int fscn = min; fscn <= max; fscn ++) {
			opcode.fscn.fscn = fscn;
			mxc_switch(opcode.line, opcode.bus, type);
			opnr ++;
		}
	}
	else {
		int min = opcode.line;
		int max = target.line;

		for(int line = min; line <= max; line ++) {
			mxc_switch(line, opcode.bus, type);
			opnr ++;
		}
	}

	umx_busy = 0;
}

void umx_init(void)
{
	board_init();
	mxc_init();
}

void umx_update(void)
{
	sys_update();
}

void vm_wait(int ms)
{
	time_t deadline = time_get(ms);
	do {
		umx_update();
	} while(time_left(deadline) > 0);
}

/*no busy return 0*/
static int umx_is_busy(void)
{
	int busy = umx_busy;
	if(!busy) busy += vm_is_busy();
	return busy;
}

static void umx_abort(void)
{
	vm_abort();
	irc_error_clear();
}

int main(void)
{
	sys_init();
	led_flash(LED_GREEN);
	printf("umx sw v1.0, build: %s %s\n\r", __DATE__, __TIME__);
	umx_init();
	vm_init();

	while(1) {
		umx_update();
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
		"*ERR? [slot]	show error code & info\n"
		"*RST		instrument reset\n"
		"*CLS		clear status & error queue, excute continues\n"
		"ABORT		abort current fail operation\n"
	};

	int ecode = 0;
	if(!strcmp(argv[0], "*IDN?")) {
		printf("<Linktron Technology,IRT16X3254,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*OPC?")) {
		int opc = !umx_is_busy();
		printf("<%+d\n\r", opc);
		return 0;
	}
	else if(!strcmp(argv[0], "*ERR?")) {
		irc_error_pop_print_history();
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		board_reset();
	}
	else if(!strcmp(argv[0], "*CLS")) {
		irc_error_clear();
		irc_error_print(IRT_E_OK);
		return 0;
	}
	else if(!strcmp(argv[0], "ABORT")) {
		umx_abort();
		irc_error_print(IRT_E_OK);
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
