/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "cmd.h"

static int shell_ReadLine(void);
static void shell_Parse(void);

static char cmd_buffer[CONFIG_SHELL_LEN_CMD_MAX]; /*max length of a cmd and paras*/
static int cmd_idx;
static char *argv[CONFIG_SHELL_NR_PARA_MAX]; /*max number of para, include command itself*/
static int argc;

void shell_Init(void)
{
	cmd_buffer[0] = 0;
	cmd_idx = -1;
	
	putchar(0x1b); /*clear screen*/
	putchar('c');
	
	cmd_Init();
	printf("%s\n", CONFIG_BLDC_BANNER);
}

void shell_Update(void)
{
	int ok;
	
	cmd_Update();
	ok = shell_ReadLine();
	if(!ok) return;
	
	shell_Parse();
	cmd_Exec(argc, argv);
}

/*read a line of string from console*/
static int shell_ReadLine(void)
{
	int ch;
	int ready = 0;
	
	if(cmd_idx == -1) { 
		printf("%s", CONFIG_SHELL_PROMPT);
		cmd_idx ++;
	}
	
	while(console_IsNotEmpty())
	{
		ch = console_getch();

		switch(ch) {
		case '\n':		// NewLine
			continue;
		case 3: /*ctrl-c*/
			strcpy(cmd_buffer, "pause");
			cmd_idx = 5;
		case '\r':		// Return
			cmd_buffer[cmd_idx] = '\0';
			cmd_idx = -1;
			ready = 1;
			putchar('\n');
			break;
		case 127:			// Backspace
			if(cmd_idx > 0)
			{
				cmd_buffer[--cmd_idx] = 0;
				/*printf("%s", "\b \b");*/
				putchar(ch);
			}
			continue;
		default:
			if((ch < ' ') || (ch > 126))
				continue;
			if(cmd_idx < CONFIG_SHELL_LEN_CMD_MAX - 1)
			{
				putchar(ch);
				cmd_buffer[cmd_idx++] = ch;
			}
			continue;
		}
	}
	
	return ready;
}

static void shell_Parse(void)
{
	int len, i = 0;
	char ch;
	int flag = 0;
	
	argc = 0;
	len = strlen(cmd_buffer);
	ch = cmd_buffer[i];
	while(ch) {
		if(ch != ' ') {
			if(argc > CONFIG_SHELL_NR_PARA_MAX)
				break;
			
			if(flag == 0) {
				argv[argc] = cmd_buffer + i;
				argc ++;
				flag = 1;
			}
		}
		else {
			cmd_buffer[i] = 0;
			flag = 0;
		}
			
		i++;
		
		if(i < len)
			ch = cmd_buffer[i];
		else
			break;
	}
	
	/*debug*/
#if 0
	for(i = 0; i < argc; i++)
		printf("argv[%d] = %s\n", i, argv[i]);
#endif
}
