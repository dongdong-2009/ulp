/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"

static cmd_list_t *cmd_list;

void cmd_Init(void)
{
	cmd_list = 0;
	cmd_Add(&cmd_help);
	cmd_Add(&cmd_speed);
	cmd_Add(&cmd_rpm);
	cmd_Add(&cmd_motor);
}

void cmd_Update(void)
{
	cmd_list_t *p = cmd_list;
	
	while(p) {
		if(p->flag & CMD_FLAG_REPEAT)
			cmd_Exec(0, 0);
		p = p->next;
	}
}

void cmd_Add(cmd_t *cmd)
{
	cmd_list_t *new;
	cmd_list_t *p = cmd_list;
	
	new = malloc(sizeof(cmd_list_t));
	new->cmd = cmd;
	new->flag = 0;
	new->next = 0;
	
	while(p && p->next) p = p->next;
	if(!p) p = cmd_list = new;
	else p->next = new;
}

void cmd_Exec(int argc, char *argv[])
{
	int len = strlen(argv[0]); 
	int ret = -1;
	cmd_list_t *p = cmd_list;
	
	/*find command from cmd_list*/
	while(p) {
		if( !strcmp(p->cmd->name, argv[0])) {
			/*found it*/	
			break;
		}
		p = p->next;
	}
	
	if(!p && (len > 0)) {
		printf("Invalid Command\n");
		return;
	}
		
	ret = p->cmd->func(argc, argv);
	if(ret > 0)
		p->flag |= CMD_FLAG_REPEAT;
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

cmd_t cmd_help = {"help", cmd_help_func, "list all commands"};
