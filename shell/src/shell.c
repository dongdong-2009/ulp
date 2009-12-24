/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

static int cmd_help_func(int agrc, char *argv[]);
static int shell_ReadLine(void);
static cmd_t *shell_Parse(void);

static char cmd_buffer[CONFIG_SHELL_LEN_CMD_MAX]; /*max length of a cmd and paras*/
static int cmd_idx;
static char *argv[CONFIG_SHELL_NR_PARA_MAX]; /*max number of para, include command itself*/
static int argc;

static cmd_list_t *cmd_list;
static cmd_t cmd_help = {"help", cmd_help_func, "list all commands"};

void shell_Init(void)
{
	cmd_list = 0;
	cmd_buffer[0] = 0;
	cmd_idx = -1;
	shell_AddCmd(&cmd_help);
	
	putchar(0x1b); /*clear screen*/
	putchar('c');
	
	printf("%s\n", CONFIG_BLDC_BANNER);
}

void shell_Update(void)
{
	int ok;
	cmd_t *p;
	
	ok = shell_ReadLine();
	if(!ok) return;
	
	p = shell_Parse();
	if(p) 
		ok = p->cmd(argc, argv);
}

void shell_AddCmd(cmd_t *cmd)
{
	cmd_list_t *new;
	cmd_list_t *p = cmd_list;
	
	new = malloc(sizeof(cmd_list_t));
	new->cmd = cmd;
	new->next = 0;
	
	while(p && p->next) p = p->next;
	if(!p) p = cmd_list = new;
	else p->next = new;
}

int cmd_help_func(int argc, char *argv[])
{
	cmd_list_t *p = cmd_list;
	
	while(p) {
		printf("%s\t%s\n", p->cmd->name, p->cmd->help);
		p = p->next;
	}
	
	return 0;
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

static cmd_t *shell_Parse(void)
{
	int len, i = 0;
	char ch;
	cmd_list_t *p;
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
	
	/*find command from cmd_list*/
	p = cmd_list;
	while(p) {
		if( !strcmp(p->cmd->name, argv[0])) {
			/*found it*/
			return p->cmd;
		}
		p = p->next;
	}
	
	if(len > 0)
		printf("Invalid Command\n");
	return 0;
}
