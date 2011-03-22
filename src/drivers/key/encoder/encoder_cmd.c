/*
 * 	miaofng@2011 initial version
 */

#include "config.h"
#include "debug.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "encoder.h"

//global var
short encoder_speed;
short encoder_v_save;

static int cmd_encoder_func(int argc, char *argv[])
{
	int val, speed;
	const char *usage = { \
		"usage:\n " \
		"encoder [min max]	init encoder and dynamicly show current status\n " \
	};

	if(argc > 0) {
		if(!strcmp(argv[1], "help")) {
			printf("%s", usage);
			return 0;
		}

		encoder_v_save = 0;
		encoder_Init();
		if(argc >= 3) {
			encoder_SetRange(atoi(argv[1]), atoi(argv[2]));
		}
		return 1;
	}

	val = encoder_GetValue();
	speed = encoder_speed;
	if((int) encoder_v_save != val) {
		printf("value: %05d speed: %03d Pulse/S\n", val, speed);
		encoder_v_save = (short) val;
	}
	return 1;
}

const cmd_t cmd_encoder = {"encoder", cmd_encoder_func, "encoder debug command"};
DECLARE_SHELL_CMD(cmd_encoder)

