/*
 * 	miaofng@2013-2-16 initial version
 *	used for oid hardware version 2.0 board
 */

#include "ulp/sys.h"
#include "oid_gui.h"

struct oid_config_s oid_config;
int oid_stm;

void oid_mdelay(int ms)
{
	sys_mdelay(ms);
}

int oid_wait(void)
{
	oid_config.start = 0;
	oid_config.pause = 0;
	for(int i = oid_config.seconds; i > 0; i --) {
		oid_show_progress(PROGRESS_BUSY);

		//wait 1s
		time_t deadline = time_get(1000);
		while((time_left(deadline) > 0) || (oid_config.pause == 1)) {
			sys_update();
			gui_update();
			if(oid_config.start == 1) { //start key is pressed again, give up the test ...
				return 1;
			}
		}
	}
	return 0;
}

void oid_init(void)
{
	oid_config.lines = 4;
	oid_config.ground = "?";
	oid_config.mode = 'i';
	oid_config.seconds = 30;
	oid_config.start = 0;
	oid_stm = IDLE;
}

void oid_update(void)
{
	if(oid_config.start == 0)
		return;

	oid_stm = BUSY;
	oid_error_init();
	oid_show_progress(PROGRESS_START);
	oid_show_result(0, 0);

	int giveup = 0;
	while((!giveup) && (oid_stm < FINISH)) {
		oid_stm ++;
		switch(oid_stm) {
		case CAL:
			//giveup = oid_instr_cal();
			break;

		case COLD:
			//giveup = oid_cold_test(&oid_config);
			break;

		case WAIT:
			giveup = oid_wait();
			break;

		case HOT:
			//giveup = oid_hot_test(&oid_config);
			break;

		default:
			giveup = 1;
			break;
		}
	}

	if(giveup) {
		oid_error_flash();
	}

	oid_show_progress(PROGRESS_STOP);
	oid_config.start = 0;
	oid_stm = IDLE;
}

void main(void)
{
	sys_init();
	oid_init();
	oid_gui_init();
	while(1) {
		sys_update();
		oid_update();
		gui_update();
	}
}
