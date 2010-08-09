/*
 * 	dusk@2010 initial version
 *	miaofng@2010 reused for common lcd debug purpose
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"

static int cmd_lcd_func(int argc, char *argv[])
{
	int row, col;
	
	const char *usage = {
		"usage:\n" \
		"lcd init\n"
		"lcd puts x y str\n"
	};
	
	if(argc == 2) {
		lcd_init();
		return 0;
	}
	else if(argc > 2) {
		row = atoi(argv[2]);
		col = atoi(argv[3]);
		lcd_puts(row, col, argv[4]);
		return 0;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_lcd = {"lcd", cmd_lcd_func, "lcd driver debug"};
DECLARE_SHELL_CMD(cmd_lcd)
