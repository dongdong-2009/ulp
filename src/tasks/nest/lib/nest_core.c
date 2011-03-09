/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "nest.h"
#include "sys/task.h"
#include <sys/sys.h>
#include <shell/cmd.h>
#include "debug.h"
#include "time.h"

static struct nest_info_s nest_info;

int nest_init(void)
{
	task_Init();
	cncb_init();
	nest_message_init();
	return 0;
}

int nest_update(void)
{
	task_Update();
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
	time_t deadline = time_get(1000 * 60 * 3);

	err = nest_error_get() -> type;
	light = (nest_pass()) ? PASS_CMPLT_ON : FAIL_ABORT_ON;
	nest_light(light);

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

//nest shell command
static int cmd_nest_func(int argc, char *argv[])
{
	const char *usage = {
		"nest help\n"
		"nest log	print log message\n"
	};

	if(argc > 1) {
		if(!strcmp(argv[1], "log"))
			return cmd_nest_log_func(argc, argv);
	}

	printf("%s", usage);
	return 0;
}

const static cmd_t cmd_nest = {"nest", cmd_nest_func, "nest debug command"};
DECLARE_SHELL_CMD(cmd_nest)
