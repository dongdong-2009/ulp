/*
 * 	dusk@2010 initial version
 *	miaofng@2010 reused for common lcd debug purpose
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pd.h"
#include "lcd.h"

int cmd_pd_get(int argc, char *argv[])
{
	int x, y, z;

	if(argc != 0) {
		if(strcmp(argv[1], "get"))
			return 0;
	}

	z = pdd_get(&x, &y);
	printf("pd: %d = pdd_get(%d, %d)\n", z, x, y);
	return 1;
}

int cmd_pd_set(int argc, char *argv[])
{
	if(argc != 0) {
		if(strcmp(argv[1], "set"))
			return -1;
	}

	switch( argc ) {
	case 5: //pd set dx dy zl
		pd_zl = atoi(argv[4]);
	case 4: //pd set dx dy
		pd_dy = atoi(argv[3]);
	case 3: //pd set dx
		pd_dx = atoi(argv[2]);
	case 2: //pd set
		break;
	default:
		return -1;
	}

	printf("pd: dx = %d dy = %d dz = %d\n", pd_dx, pd_dy, pd_zl);
	return 0;
}

static int cmd_pd_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"pd init\n"
		"pd get\n"
		"pd set dx dy zl\n"
	};

	if(!strcmp(argv[1], "init")) {
		pd_Init();
		return 0;
	}

	if(!cmd_pd_set(argc, argv))
		return 0;
	if(cmd_pd_get(argc, argv) > 0)
		return 1;

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_pd = {"pd", cmd_pd_func, "touch screen driver debug"};
DECLARE_SHELL_CMD(cmd_pd)
