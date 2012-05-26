/*
 *	david@2012 pwm output tools initial version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "shell/cmd.h"
#include "ulp_time.h"
#include "sys/sys.h"
#include "counter.h"

static int cmd_counter_func(int argc, char *argv[])
{
	int result = 0, temp;
	static int cnt;
	static const counter_bus_t *counter;

	const char *usage = {
		"usage:\n"
		"counter chxx get	get chxx(ch11...ch42) pulse input\n"
	};

	if (argc == 3) {
		temp = atoi(argv[1] + 2);
		switch (temp) {
		case 11:
			counter = &counter11;
			result = 1;
			break;
		case 12:
			counter = &counter12;
			result = 1;
			break;
		case 21:
			counter = &counter21;
			result = 1;
			break;
		case 22:
			counter = &counter22;
			result = 1;
			break;
		default:
			break;
		}
		if (result) {
			counter->init(NULL);
			return 1;
		}
	}
	if (argc == 0) {
		if(cnt != counter->get()) {
			cnt = counter->get();
			printf("current counter: %d\n", cnt);
		}
		return 1;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_counter = {"counter", cmd_counter_func, "counter utility tools"};
DECLARE_SHELL_CMD(cmd_counter)
