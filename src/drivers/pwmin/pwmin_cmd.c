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
#include "pwmin.h"

static int cmd_pwmin_func(int argc, char *argv[])
{
	int result = 0, temp;
	pwmin_cfg_t cfg;
	const pwmin_bus_t *pwmin;
	cfg.sample_clock = 1000000;

	const char *usage = {
		"usage:\n"
		"pwmin chxx sc init, init pwmin(ch11...ch41) sample clock\n"
		"pwmin chxx dc, get the duty cycle of input pwm\n"
		"pwmin chxx frq, get the frequence of input pwm\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (argc > 1) {
		temp = atoi(argv[1] + 2);

		switch (temp) {
		case 11:
			pwmin = &pwmin11;
			result = 1;
			break;
		default:
			break;
		}

		if (result) {
			if (strcmp(argv[3], "init") == 0) {
				cfg.sample_clock = atoi(argv[2]);
				pwmin->init(&cfg);
			}
			if (strcmp(argv[2], "dc") == 0) {
				printf("Duty Cycle : %d\n", pwmin->getdc());
			}
			if (strcmp(argv[2], "frq") == 0) {
				printf("Frequence : %d\n", pwmin->getfrq());
			}
		}
	}

	return 0;
}

const cmd_t cmd_pwmin = {"pwmin", cmd_pwmin_func, "pwm input utility tools"};
DECLARE_SHELL_CMD(cmd_pwmin)
