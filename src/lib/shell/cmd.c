/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell/cmd.h"
#include "sys/sys.h"
#include "debug.h"

/*private*/
static struct cmd_queue_s *cmd_queue;

void cmd_Init(void)
{
	cmd_queue = NULL;
}

void cmd_Update(void)
{
}

static cmd_t *__name2cmd(const char *name)
{
	cmd_t **entry, **end;
	if(name != NULL) {
		entry = __section_begin(".shell.cmd");
		end = __section_end(".shell.cmd");
		while(entry < end) {
			if(!strcmp(name, (*entry)->name))
				return *entry;
			entry ++;
		}
	}

	return NULL;
}

static int __cmd_parse(char *cmdline, int len, char **argv, int n)
{
	int i, argc, flag;

	//parse the Command Line
	argc = 0;
	for(flag = i = 0; i < len; i ++) {
		if(cmdline[i] == ' ') {
			cmdline[i] = 0;
			flag = 0;
		}
		else if(flag == 0) { //new argv
			if(argc < n && argv != NULL) {
				argv[argc] = cmdline + i;
				argc ++;
				flag = 1;
			}
			else break;
		}
	}

	/*parser debug*/
#if 0
	for(i = 0; i < argc; i ++)
		printf("argv[%d] = %s\n", i, argv[i]);
#endif

	return argc;
}

static int __cmd_exec(struct cmd_list_s *clst)
{
	int argc, ret;
	char *argv[16];
	cmd_t *cmd;

	ret = 0;
	argc = __cmd_parse(clst -> cmdline, clst -> len, argv, 16);
	if(argc > 0) {
		cmd = __name2cmd(argv[0]);
		if(cmd != NULL) {
			if(list_empty(&clst -> list))
				ret = cmd -> func(argc, argv);
			else //to keep compatibility with the old call style
				ret = cmd -> func(0, argv);
		}
	}

	return ret;
}

int cmd_queue_init(struct cmd_queue_s *cq)
{
	cq -> flag = 0;
	INIT_LIST_HEAD(&cq -> cmd_list);
	return 0;
}

int cmd_queue_update(struct cmd_queue_s *cq)
{
	struct cmd_list_s *clst;
	struct list_head *pos, *bkup;

	if(cq -> flag) { //background tasks update stop
		return 0;
	}

	list_for_each_safe(pos, bkup, &cq -> cmd_list) {
		clst = list_entry(pos, cmd_list_s, list);
		if( __cmd_exec(clst) <= 0) {
			//remove from queue
			list_del(&clst -> list);
			sys_free(clst);
		}
	}
	return 0;
}

/*note: this is the common entry of all cmd_xxx_func*/
int cmd_queue_exec(struct cmd_queue_s *cq, const char *cl)
{
	int n;
	struct cmd_list_s *clst;

	//create a new clst object
	n = strlen(cl);
	clst = sys_malloc(sizeof(struct cmd_list_s) + n + 1);
	assert(cq != NULL);
	clst -> cmdline = (char *)clst + sizeof(struct cmd_list_s);
	strcpy(clst -> cmdline, cl);
	clst -> len = n;
	INIT_LIST_HEAD(&clst -> list);

	//exec clst
	cmd_queue = cq;
	if(__cmd_exec(clst) != 0) {
		//repeat, add to cmd queue
		list_add(&cq -> cmd_list, &clst -> list);
	}
	else
		sys_free(clst);
	return 0;
}

int cmd_help_func(int argc, char *argv[])
{
	cmd_t **entry, **end, *cmd;

	entry = __section_begin(".shell.cmd");
	end = __section_end(".shell.cmd");
	while(entry < end) {
		cmd = *entry;
		printf("%s\t%s\n", cmd->name, cmd->help);
		entry ++;
	}
	return 0;
}

const cmd_t cmd_help = {"help", cmd_help_func, "list all commands"};
DECLARE_SHELL_CMD(cmd_help)

static int cmd_ListBgTasks(void)
{
	struct list_head *pos;
	struct cmd_list_s *clst;
	int i = 0;

	assert(cmd_queue != NULL);

	/*print bg task list*/
	printf("bg tasks:\n");
	list_for_each(pos, &cmd_queue -> cmd_list) {
		clst = list_entry(pos, cmd_list_s, list);
		printf("%d, %s\n", i, clst -> cmdline);
		i++;
	}

	return i;
}

static int cmd_pause_func(int argc, char *argv[])
{
	int on = 0; /*-1-> off, 1->on, 0 undef*/

	assert(cmd_queue != NULL);

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
		if(cmd_queue -> flag)
			on = -1;
		else
			on = 1;
	}

	if (on == -1) { /*pause off*/
		cmd_queue -> flag = 0;
		printf("bg task continues ...\n");
		return 0;
	}

	/*pause on*/
	cmd_ListBgTasks();

	/*pause bg tasks*/
	cmd_queue -> flag = 1;
	printf("note: bg task is paused!!!\n");

	return 0;
}

const cmd_t cmd_pause = {"pause", cmd_pause_func, "pause all background task, hotkey: ctrl-c"};
DECLARE_SHELL_CMD(cmd_pause)

static int cmd_kill_func(int argc, char *argv[])
{
	int i, notfound;
	struct list_head *pos, *bkup;
	struct cmd_list_s *clst;
	char *cmd_name;
	const char *usage =  { \
		"usage:\n" \
		" kill cmd_name cmd_name ...\n" \
		" kill all\n" \
	};

	assert(cmd_queue != NULL);

	if(argc < 2) {
		printf("%s",usage);
		cmd_ListBgTasks();
		return 0;
	}

	for(i = 1; i < argc; i++) {
		notfound = 1;
		list_for_each_safe(pos, bkup, &cmd_queue -> cmd_list) {
			clst = list_entry(pos, cmd_list_s, list);
			if(__cmd_parse(clst -> cmdline, clst -> len, &cmd_name, 1) > 0) {
				notfound = strcmp(argv[i], cmd_name); /*match the task name*/
			}

			if(notfound)
				notfound = strcmp(argv[i], "all");

			if(!notfound) { /*found*/
				list_del(&clst -> list);
				sys_free(clst);
				break;
			}
		}

		if(notfound) { /*invalid task name para*/
			printf("%s",usage);
			cmd_ListBgTasks();
			break;
		}
	}
	return 0;
}

const cmd_t cmd_kill = {"kill", cmd_kill_func, "kill a background task"};
DECLARE_SHELL_CMD(cmd_kill)
