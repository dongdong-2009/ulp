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
		"lcd w reg val\n"
		"lcd r reg\n"
	};
	
	if(argc == 2) {
		lcd_init();
		return 0;
	}
	else if(argc > 2) {
		if(argv[1][0] == 'w') {
			//write reg
			sscanf(argv[2], "%x", &row);
			sscanf(argv[3], "%x", &col);
			lcd_writereg(row, col);
			printf("reg[0x%x] = 0x%x)\n", row, col);
			return 0;
		}
		
		if(argv[2][0] == 'r') {
			sscanf(argv[2], "%x", &row);
			col = lcd_readreg(row);
			printf("reg[0x%x] = 0x%x)\n", row, col);
			return 0;
		}
		
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
