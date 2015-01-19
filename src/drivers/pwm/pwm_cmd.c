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
#include "pwm.h"

static int cmd_pwm_func(int argc, char *argv[])
{
	int freq = 1000, dc = 50, result = 0, temp;
	pwm_cfg_t cfg;
	const pwm_bus_t *pwm;
	cfg.fs = 100;

	const char *usage = {
		"usage:\n"
		"pwm chxx frq dc,	set pwm(ch11...ch44) frequence and duty cycle\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (argc == 4) {
		temp = atoi(argv[1] + 2);
		freq = atoi(argv[2]);
		dc = atoi(argv[3]);

		switch (temp) {
#ifdef CONFIG_DRIVER_PWM3
		case 31:
			pwm = &pwm31;
			result = 1;
			break;
		case 32:
			pwm = &pwm32;
			result = 1;
			break;
		case 33:
			pwm = &pwm33;
			result = 1;
			break;
		case 34:
			pwm = &pwm34;
			result = 1;
			break;
#endif
#ifdef CONFIG_DRIVER_PWM4
		case 41:
			pwm = &pwm41;
			result = 1;
			break;
		case 42:
			pwm = &pwm42;
			result = 1;
			break;
		case 43:
			pwm = &pwm43;
			result = 1;
			break;
		case 44:
			pwm = &pwm44;
			result = 1;
			break;
#endif
		default:
			break;
		}

		if (result) {
			cfg.hz = freq;
			pwm->init(&cfg);
			pwm->set(dc);
		}
	}

	return 0;
}

const cmd_t cmd_pwm = {"pwm", cmd_pwm_func, "pwm utility tools"};
DECLARE_SHELL_CMD(cmd_pwm)
