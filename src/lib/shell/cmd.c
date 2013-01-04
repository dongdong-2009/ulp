/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell/cmd.h"
#include "sys/sys.h"
#include "ulp/debug.h"

/*private*/
static struct cmd_queue_s *cmd_queue;

void cmd_Init(void)
{
	cmd_queue = NULL;
}

void cmd_Update(void)
{
}

#ifdef CONFIG_CMD_PATTERN
/*get pattern from a string, such as: 0,2-5,8*/
int cmd_pattern_get(const char *str)
{
	char *p;
	int n, pt = 0, start, stop, tmp, inverse = 0;

	for(;(str != NULL) && (*str != 0); str = p) {
		/*pre-process*/
		for(;*str == ','; str ++); /*ignore ',' on head*/
		p = strchr(str, ','); /*record  next segment position*/
		if(!strncmp(str, "all", 3)) {
			pt = -1;
			inverse = 1; /*inverse selection*/
			continue;
		}

		/*process current segment*/
		n = sscanf(str, "%d-%d,", &start, &stop);
		if(n > 0) {
			stop = (n == 1) ? start : stop;
			start = (start > 31) ? 31 : start;
			start = (start < 0) ? 0 : start;
			stop = (stop > 31) ? 31 : stop;
			stop = (stop < 0) ? 0 : stop;
			if(start > stop) { /*swap*/
				tmp = start;
				start = stop;
				stop = tmp;
			}

			for(;start <= stop; start ++) {
				pt = (inverse == 0) ? (pt | (1 << start)) : (pt & (~(1 << start)));
			}
		}
	}
	return pt;
}
#endif

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

/*to call cmd like: help& or help&100*/
void __cmd_preprocess(struct cmd_list_s *clst)
{
#ifdef CONFIG_CMD_BKG
	char *p;
	int i;

	clst -> ms = -1;
	for( i = clst -> len; i > 0; i --) {
		p = clst -> cmdline + i;
		if(*p == '&') {
			clst -> len = i; //new clst cmdline length
			*p ++ = 0;
			clst -> ms = 0;
			if(*p != 0) {
				clst -> ms = atoi(p);
			}
			break;
		}
	}
#endif
}

static int __cmd_parse(char *cmdline, int len, char **argv, int n)
{
	int i, argc, flag, mode_str = 0;
#ifdef CONFIG_CMD_BKG
	int mode_func = 0;
#endif
	//parse the Command Line
	argc = 0;
	for(flag = i = 0; i < len; i ++) {
		switch(cmdline[i]) {
		case ' ':
		case '	':
		case 0:
			if(mode_str)
				continue;
			cmdline[i] = 0;
			flag = 0;
			continue;
#ifdef CONFIG_CMD_FUNCTION
		case '(':
			if(mode_str)
				continue;
			if(argc == 1) {
				flag = 0;
				mode_func = 1;
				cmdline[i] = 0;
				continue;
			}
			else break;

		case ',':
			if(mode_str)
				continue;
			if(mode_func == 1) { //function mode
				flag = 0;
				cmdline[i] = 0;
				continue;
			}
			else break;

		case ')':
			if(mode_str)
				continue;
			if(mode_func == 1) { //function mode
				flag = 0;
				cmdline[i] = 0;
				i = len; //jump out
				continue;
			}
			else break;
#endif

		case '\'':
		case '"':
			if(mode_str && mode_str == cmdline[i]) {
				mode_str = 0;
				flag = 0;
				cmdline[i] = 0;
				continue;
			}
			else if(mode_str == 0) {
				mode_str = cmdline[i];
				cmdline[i] = 0;
				if(i + 1 < len) {
					flag = 0;
					i ++;
				}
				break;
			}
			continue;

		default:
			break;
		}

		if(flag == 0) { //new argv
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

enum {
	__CMD_EXEC_FLAG_INIT,
	__CMD_EXEC_FLAG_UPDATE,
	__CMD_EXEC_FLAG_CLOSE,
};

static int __cmd_exec(struct cmd_list_s *clst, int flag)
{
	int argc, ret, n;
	char *argv[CONFIG_SHELL_NR_PARA_MAX], **_argv = argv;
	cmd_t *cmd;

	cmd = cmd_queue->trap;
	if(cmd != NULL) {
		n = strlen(cmd->name);
		if(strncmp(clst->cmdline, cmd->name, n)) {
			argc = 1;
			argv[0] = clst->cmdline;
			ret = cmd->func(argc, argv);
			return ret;
		}
	}

	ret = 0;
	argc = __cmd_parse(clst -> cmdline, clst -> len, argv, CONFIG_SHELL_NR_PARA_MAX);
	if(argc > 0) {
		cmd = (cmd_queue->trap != NULL) ? cmd_queue->trap : __name2cmd(argv[0]);
		if(cmd != NULL) {
#ifdef CONFIG_CMD_BKG
			if(clst -> ms >= 0) {
				clst -> deadline = time_get(clst -> ms);
			}
#endif
			switch (flag) {
			case __CMD_EXEC_FLAG_CLOSE:
				_argv = NULL;
			case __CMD_EXEC_FLAG_UPDATE:
				argc = 0;
			default:
				break;
			}

			ret = cmd -> func(argc, _argv);
		}
	}

	return ret;
}

int cmd_queue_init(struct cmd_queue_s *cq)
{
	cq -> flag = 0;
	cq -> trap = NULL;
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
#ifdef CONFIG_CMD_BKG
		//timeout check
		if(clst -> ms > 0) {
			if(time_left(clst -> deadline) > 0)
				continue;
		}
#endif
		if( __cmd_exec(clst, __CMD_EXEC_FLAG_UPDATE) <= 0) {
#ifdef CONFIG_CMD_BKG
			//remove from queue
			if(clst -> ms < 0) {
				list_del(&clst -> list);
				sys_free(clst);
			}
#endif
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
	__cmd_preprocess(clst);

	//exec clst
	cmd_queue = cq;
	if(__cmd_exec(clst, __CMD_EXEC_FLAG_INIT) > 0
#ifdef CONFIG_CMD_BKG
		|| clst -> ms >= 0
#endif
	) {
		//repeat, add to cmd queue
		list_add(&clst -> list, &cq -> cmd_list);
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
#ifdef CONFIG_CMD_BKG
		if(clst -> ms < 0)
			printf("%d, %s\n", i, clst -> cmdline);
		else if(clst -> ms > 0)
			printf("%d, %s&%d\n", i, clst -> cmdline, clst -> ms);
		else
#endif
			printf("%d, %s&\n", i, clst -> cmdline);
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
				__cmd_exec(clst, __CMD_EXEC_FLAG_CLOSE);
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

static int cmd_echo_func(int argc, char *argv[])
{
	for(int i = 0; i < argc; i ++) {
		printf("argv[%d] = %s\n", i, argv[i]);
	}
	return 0;
}

const cmd_t cmd_echo = {"echo", cmd_echo_func, "echo the cmdline"};
DECLARE_SHELL_CMD(cmd_echo)
