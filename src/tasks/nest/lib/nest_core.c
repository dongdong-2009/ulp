/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "nest.h"
#include "sys/task.h"
#include <sys/sys.h>
#include <shell/cmd.h>
#include "ulp/debug.h"
#include "ulp_time.h"
#include "nvm.h"

static int nest_flag_ignore __nvm;
static struct nest_info_s nest_info;
#ifdef CONFIG_NEST_ID
static int nest_id __nvm;
#endif
#ifdef CONFIG_NEST_MT80_OLD
int write_nest_psv(int argc, char *cond_flag[]);
#endif

int nest_init(void)
{
	task_Init();
	cncb_init();
	nest_message_init();
	nest_wl_init();
	if(nest_flag_ignore == -1)
		nest_flag_ignore = 0;
#ifdef CONFIG_NEST_ID
	nest_info.id_base = nest_id;
#endif
	nest_message("$nest_id = %d\n", nest_info.id_base);
	nest_message("$nest_ignore = 0x%04x\n", nest_flag_ignore);
	return 0;
}

int nest_update(void)
{
	task_Update();
	nest_wl_update();
	return 0;
}

int nest_wait_plug_in(void)
{
	nest_message("Waiting for Module Insertion ... ");
	do {
		while (cncb_detect(1)) {
			nest_update();
		}
	} while(cncb_detect(500));
	nest_message("Detected!!!\n");

	nest_message_init();
	nest_message("#Test Start\n");
	nest_time_init();
	nest_error_init();
	return 0;
}

int nest_wait_pull_out(void)
{
	int light, err;
	time_t timer_restart = time_get(1000 * 60 * 3);

	err = nest_error_get() -> type;
	light = (nest_pass()) ? PASS_CMPLT_ON : FAIL_ABORT_ON;
	nest_light(light);

#if CONFIG_NEST_LOG_SIZE > 0
	//save the log message to flash
	if(nest_fail())
		nvm_save();
#endif

	while(! cncb_detect(0)) {
		nest_light_flash(err);
#ifdef CONFIG_NEST_AUTORESTART
		if(time_left(timer_restart) < 0) {
			if(nest_pass())
				break;
		}
#endif
	}

	nest_light(ALL_OFF);
	return 0;
}

int nest_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			nest_update();
		}
	} while(left > 0);

	return 0;
}

struct nest_info_s* nest_info_get(void)
{
	return &nest_info;
}

int nest_map(const struct nest_map_s *map, const char *str)
{
	int id, len;
	assert(map != NULL);

	for(id = -1; map -> str != NULL; map ++) {
		len = strlen(map -> str);
		if(!strncmp(map -> str, str, len)) {
			id = map -> id;
			break;
		}
	}
	return id;
}

int nest_ignore(int mask)
{
	return (nest_flag_ignore & mask);
}

//nest shell command
static int cmd_nest_func(int argc, char *argv[])
{
	const char *usage = {
		"nest help\n"
		"nest save                              save settings to nvm\n"
		"nest log                               print log message\n"
		"nest ignore psv bmr rly pkt all non    ignore some event for debug\n"
		"nest id                                set nest id eg.(001)\n"
		"nest cond_flag                         write nest cond_flag by hex eg.(0xff)\n"
	};

	if(argc > 1) {
		if(!strcmp(argv[1], "save")) {
			nvm_save();
			return 0;
		}
		if(!strcmp(argv[1], "log"))
			return cmd_nest_log_func(argc, argv);
		if(!strcmp(argv[1], "ignore")) {
			int ignore = 0, ok = 1;
			for(int i = 2; i < argc; i ++) {
				if(!strcmp(argv[i], "psv")) {
					ignore |= PSV;
				}
				else if(!strcmp(argv[i], "bmr")) {
					ignore |= BMR;
				}
				else if(!strcmp(argv[i], "rly")) {
					ignore |= RLY;
				}
				else if(!strcmp(argv[i], "pkt")) {
					ignore |= PKT;
				}
				else if(!strcmp(argv[i], "all")) {
					ignore |= (PSV | BMR | RLY | PKT);
				}
				else if(!strcmp(argv[i], "non")) {
					ignore = 0;
				}
				else {
					ok = 0;
					break;
				}
			}

			//display current settings
			if(ok) {
				if(argc > 2)
					nest_flag_ignore = ignore;
				nest_message("nest_ignore = ");
				if(nest_ignore(PKT))
					nest_message("|PKT");
				else
					nest_message("|   ");
				if(nest_ignore(RLY))
					nest_message("|RLY");
				else
					nest_message("|   ");

				if(nest_ignore(BMR))
					nest_message("|BMR");
				else
					nest_message("|   ");

				if(nest_ignore(PSV))
					nest_message("|PSV");
				else
					nest_message("|   ");
				nest_message("|\n");
				return 0;
			}
		}
#ifdef CONFIG_NEST_ID
		if(!strcmp(argv[1], "id")) {
			if(argc == 3) {
				nest_id = atoi(argv[2]);
				nest_info.id_base = nest_id;
			}
			nest_message("nest id %03d \n",nest_info.id_base);
			return 0;
		}
#endif
#ifdef CONFIG_NEST_MT80_OLD
		if(!strcmp(argv[1], "cond_flag")) {
			int fail = write_nest_psv(argc , argv);
			if(fail) return -1;
			return 0;
		}
#endif
	}
	printf("%s", usage);
	return 0;
}


const static cmd_t cmd_nest = {"nest", cmd_nest_func, "nest debug command"};
DECLARE_SHELL_CMD(cmd_nest)
