/*
 * 	dusk@2010 initial version
 *	miaofng@2010 reused for common lcd debug purpose
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lpt.h"
#include "lcd.h"

extern struct lpt_cfg_s lpt_cfg;

static int cmd_lpt_func(int argc, char *argv[])
{
	int t, tp;
	char str[64];
	struct lcd_s *lcd;
	const char *usage = {
		"usage:\n"
		"lpt t [tp]	setting bus period(ns), tp(50%? x t)\n"
	};

	if(argc >= 2) {
		t = atoi(argv[1]);
		tp = 50;
		if(argc > 2) {
			tp = atoi(argv[2]);
		}
		tp = t * tp / 100;
		sprintf(str, "lpt: t = %dns, tp = %dns", t, tp);
		printf("%s\n", str);
		lpt_cfg.t = t;
		lpt_cfg.tp = tp;
#ifdef CONFIG_DRIVER_LCD
		lcd = lcd_get(NULL);
		sprintf(str, "t=%dns, tp=%dns", t, tp);
		lcd_puts(lcd, 0, 0, str);
#endif
		return 0;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_lpt = {"lpt", cmd_lpt_func, "lpt bus configuration"};
DECLARE_SHELL_CMD(cmd_lpt)
