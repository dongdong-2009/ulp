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
#include "../mxc.h"

void dps_auto_regulate(void)
{
}

int lv_config(int key, float value)
{
	switch(key) {
	case DPS_KEY_U:
		lv_u_set(value);
		lv_enable(value > 0.1);
		break;
	default:
		break;
	}
	return 0;
}

void dps_can_handler(can_msg_t *msg)
{
	dps_cfg_msg_t *cfg = (dps_cfg_msg_t *) msg->data;
	int id = msg->id;
	id &= 0xfff0;

	if((id == CAN_ID_CFG) && (cfg->cmd == DPS_CMD_CFG)) {
		switch(cfg->dps) {
		case DPS_LV:
			lv_config(cfg->key, cfg->value);
			break;
		case DPS_HV:
			break;
		case DPS_IS:
			break;
		default:
			break;
		}
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
        led_flash(LED_RED);
	lv_enable(0);
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
