/*
*
*  miaofng@2014-6-4   initial version
*
*/
#include "ulp/sys.h"
#include "../irc.h"
#include "../err.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "can.h"
#include "dps.h"

void dps_auto_regulate(void)
{
}

void dps_can_handler(can_msg_t *msg)
{
	int id = msg->id;
	id &= 0xfff0;

	switch(id) {
	case CAN_ID_DPS:
		break;
	default:
		break;
	}
}

void dps_init(void)
{
	dps_drv_init();
}

void main()
{
	sys_init();
	dps_init();
	lv_enable(1);
	printf("dps v1.0, SW: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
	}
}

static int cmd_dps_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"dps lv	12.5		set LV output to 12.5v\n"
		"dps is	[0.5]		get or set IS output to 0.5A\n"
	};

	if(!strcmp(argv[1], "lv")) {
		if(argc > 2) {
			float v = atof(argv[2]);
			lv_u_set(v);
		}
	}
	else if(!strcmp(argv[1], "is")) {
		if(argc > 2) {
			float a = atof(argv[2]);
			is_i_set(a);
		}
		printf("IS = %.3f A\n", is_i_get());
	}
	else {
		printf("%s", usage);
	}
	return 0;
}
const cmd_t cmd_dps = {"dps", cmd_dps_func, "dps debug commands"};
DECLARE_SHELL_CMD(cmd_dps)
