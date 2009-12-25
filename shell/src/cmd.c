/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"

static cmd_list_t *cmd_list;
static char cmd_update_stop_flag;

void cmd_Init(void)
{
	cmd_list = 0;
	cmd_update_stop_flag = 0;
	
	cmd_Add(&cmd_help);
	cmd_Add(&cmd_pause);
	cmd_Add(&cmd_kill);
	cmd_Add(&cmd_speed);
	cmd_Add(&cmd_rpm);
	cmd_Add(&cmd_motor);
	cmd_Add(&cmd_pid);
	cmd_Add(&cmd_debug);
}

void cmd_Update(void)
{	
	int ret;
	cmd_list_t *p = cmd_list;
	
	if(cmd_update_stop_flag)
		return;
		
	while(p) {
		if(p->flag & CMD_FLAG_REPEAT) {
			ret = p->cmd->func(0, 0);
			if(ret != 0)
				p->flag |= CMD_FLAG_REPEAT;
			else
				p->flag &= (~CMD_FLAG_REPEAT);
		}
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
	
	if(!p) {
		if(len > 0)
			printf("Invalid Command\n");
		return;
	}
		
	ret = p->cmd->func(argc, argv);
	if(ret != 0)
		p->flag |= CMD_FLAG_REPEAT;
	else
		p->flag &= (~CMD_FLAG_REPEAT);
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

static int cmd_ListBgTasks(void)
{
	cmd_list_t *p = cmd_list;
	int i = 0;
	
	/*print bg task list*/
	while(p) {
		if(p->flag & CMD_FLAG_REPEAT) {
			if(!i)
				printf("bg tasks:\n");
			printf("%d, %s\n", i, p->cmd->name);
			i++;
		}
		p = p->next;
	}
	
	return i;
}

static int cmd_pause_func(int argc, char *argv[])
{
	int on = 0; /*-1-> off, 1->on, 0 undef*/
	
	if(argc > 1){
		if(!strcmp(argv[1], "off"))
			on = -1;
		else if (!strcmp(argv[1], "on"))
			on = 1;
		else printf( \
			"usage:\n" \
			" pause on/off\n" \
		);
	}
	
	if(!on) {
		if(cmd_update_stop_flag)
			on = -1;
		else
			on = 1;
	}
	
	if (on == -1) { /*pause off*/
		cmd_update_stop_flag = 0;
		printf("bg task continues ...\n");
		return 0;
	}
	
	/*pause on*/
	cmd_ListBgTasks();
	
	/*pause bg tasks*/
	cmd_update_stop_flag = 1;
	printf("note: bg task is paused!!!\n");
	
	return 0;
}

cmd_t cmd_pause = {"pause", cmd_pause_func, "pause all background task, hotkey: ctrl-c"};

static int cmd_kill_func(int argc, char *argv[])
{
	int i, notfound;
	cmd_list_t *p = cmd_list;
	
	char *usage =  { \
		"usage:\n" \
		" kill cmd_name cmd_name ...\n" \
		" kill all\n" \
	};
	
	if(argc < 2) {
		printf("%s",usage);
		cmd_ListBgTasks();
		return 0;
	}
	
	for(i = 1; i < argc; i++) {
		notfound = 1;
		
		/*find from bg task list*/
		while(p) {
			if(p->flag & CMD_FLAG_REPEAT) {
				/*match the task name*/
				notfound = strcmp(argv[i], p->cmd->name);
				if(notfound)
					notfound = strcmp(argv[i], "all");
				
				if(!notfound) { /*found*/
					p->flag &= ~CMD_FLAG_REPEAT;
					break;
				}
			}
			p = p->next;
		}
		
		/*invalid task name para*/
		if(notfound) {
			printf("%s",usage);
			cmd_ListBgTasks();
			break;
		}
	}
	
	return 0;
}

cmd_t cmd_kill = {"kill", cmd_kill_func, "kill a background task"};
