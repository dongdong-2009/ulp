/*
 * 	dusk@2010 initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/system.h"
#include "stm32f10x.h"
#include "lcd1602.h"

/*for lcd1602 init*/
static int cmd_lcd_init(int argc, char *argv[])
{
	if(argc != 1){
		printf("uasge:\n");
		printf("lcdinit\n");
		return 0;
	}
	
	while(lcd1602_Init());
	printf("lcd1602 init ok \n");
	
	return 0;
}
const cmd_t cmd_lcdinit = {"lcdinit", cmd_lcd_init, "for lcd1602 init"};
DECLARE_SHELL_CMD(cmd_lcdinit)

/*for lcd putstring*/
static int cmd_lcd_puts(int argc, char *argv[])
{
	int row;
	
	if(argc != 3){
		printf("uasge:\n");
		printf("lcdputs row(0,1) string\n");
		return 0;
	}
	
	sscanf(argv[1],"%d",&row);
	
	lcd1602_WriteString(0,row,argv[2]);
	
	return 0;
}
const cmd_t cmd_lcdputs = {"lcdputs", cmd_lcd_puts, "for lcd put string"};
DECLARE_SHELL_CMD(cmd_lcdputs)
