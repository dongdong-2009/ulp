/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vvt/vvt.h"
#include "vvt/misfire.h"
#include "pss.h"
#include "knock.h"

#if CONFIG_SHELL_VVT_DEBUG == 1
static int cmd_pss_func(int argc, char *argv[])
{
	int result = -1;
	short speed, ch, mv;
	
	vvt_Stop();
	if(!strcmp(argv[1], "speed") && (argc == 3)) {
		speed = (short)atoi(argv[2]);
		pss_SetSpeed(speed);
		result = 0;
	}
	
	if(!strcmp(argv[1], "volt") && (argc == 4)) {
		ch = (short)atoi(argv[2]);
		mv = (short)atoi(argv[3]);
		ch = (ch > 3) ? 3 : ch;
		pss_SetVolt((pss_ch_t)ch, mv);
		result = 0;
	}
	
	if(result == -1) {
		printf("uasge:\n");
		printf(" pss speed 100\n");
		printf(" pss volt 0 0\n");
	}
	
	return 0;
}

cmd_t cmd_pss = {"pss", cmd_pss_func, "debug pss driver of vvt"};
DECLARE_SHELL_CMD(cmd_pss)

static int cmd_knock_func(int argc, char *argv[])
{
	int result = -1;
	short hz, ch, mv;
	
	vvt_Stop();
	if(!strcmp(argv[1], "freq") && (argc == 3)) {
		hz = (short)atoi(argv[2]);
		knock_SetFreq(hz);
		result = 0;
	}
	
	if(!strcmp(argv[1], "volt") && (argc == 4)) {
		ch = (short)atoi(argv[2]);
		mv = (short)atoi(argv[3]);
		ch = (ch > 3) ? 3 : ch;
		knock_SetVolt((knock_ch_t)ch, mv);
		result = 0;
	}
	
	if(result == -1) {
		printf("uasge:\n");
		printf(" knock freq 100\n");
		printf(" knock volt 0 0\n");
	}
	
	return 0;
}

cmd_t cmd_knock = {"knock", cmd_knock_func, "debug knock driver of vvt"};
DECLARE_SHELL_CMD(cmd_knock)
#endif

static int cmd_vvt_func(int argc, char *argv[])
{
	int result = -1;
	short speed, s, a, b, c, d;
	
	vvt_Start();
	
	if(!strcmp(argv[1], "speed") && (argc == 3)) {
		speed = (short)atoi(argv[2]);
		misfire_SetSpeed(speed);
		result = 0;
	}
	
	if(!strcmp(argv[1], "advance") && (argc == 3)) {
		vvt_gear_advance = (short)atoi(argv[2]);
		result = 0;
	}
	
	if(!strcmp(argv[1], "misfire") && (argc == 7)) {
		s = (short)(atof(argv[2]) * 32768);
		a = (short) atoi(argv[3]);
		b = (short) atoi(argv[4]);
		c = (short) atoi(argv[5]);
		d = (short) atoi(argv[6]);
		
		a = (a > 0) ? 1 : 0;
		b = (a > 0) ? 2 : 0;
		c = (a > 0) ? 4 : 0;
		d = (a > 0) ? 8 : 0;
		
		misfire_Config(s, a+b+c+d);
		result = 0;
	}
	
	if(!strcmp(argv[1], "knock") && (argc == 5)) {
		vvt_knock_pos = (short) atoi(argv[2]);
		vvt_knock_width = (short) atoi(argv[3]);
		vvt_knock_strength = (short) atoi(argv[4]);
		result = 0;
	}
	
	if(!strcmp(argv[1], "kpat") && (argc == 6)) {
		a = (short) atoi(argv[2]);
		b = (short) atoi(argv[3]);
		c = (short) atoi(argv[4]);
		d = (short) atoi(argv[5]);
		
		a = (a > 0) ? 1 : 0;
		b = (a > 0) ? 2 : 0;
		c = (a > 0) ? 4 : 0;
		d = (a > 0) ? 8 : 0;
		vvt_knock_pattern = a + b + c + d;
		result = 0;
	}
	
	if(result == -1) {
		printf("uasge:\n");
		printf(" vvt speed 100	unit: rpm\n");
		printf(" vvt advance 5	unit: gears, range: 0~10 \n");
		printf(" vvt misfire s A B C D 	strength(0.0~1)\n");
		printf(" vvt knock p w s	pos,width,strength(mV)\n");
		printf(" vvt kpat A B C D	knock pattern\n");
	}
	
	return 0;
}

cmd_t cmd_vvt = {"vvt", cmd_vvt_func, "commands for vvt"};
DECLARE_SHELL_CMD(cmd_vvt)
