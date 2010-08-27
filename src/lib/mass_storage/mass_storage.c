/*
 *	dusk@initial version
 */

#include "stm32f10x.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "platform_config.h"
#include "usb_pwr.h"
#include "mass_mal.h"
#include "usb.h"
#include <stdio.h>
#include "sys/task.h"

void mass_storage_Init(void)
{
	MALO_Init(0);
	usblower_Init();

	//init usb stack
	USB_Init();
	while(bDeviceState != CONFIGURED);
	printf("the usb device has been connected to PC Host!\n\r");
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_udisk_func(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" udisk init, enable mass storage \n" \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if(!strcmp(argv[1], "init"))
		mass_storage_Init();

	if(!strcmp(argv[1], "remove"))
		usblower_PullupDisable();

	return 0;
}
const cmd_t cmd_udisk = {"udisk", cmd_udisk_func, "udisk cmd"};
DECLARE_SHELL_CMD(cmd_udisk)
#endif

