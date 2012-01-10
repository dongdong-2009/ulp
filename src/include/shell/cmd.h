/*
 * 	miaofng@2009 initial version
 */
#ifndef __CMD_H_
#define __CMD_H_

#include "ulp_time.h"
#include <stdio.h>
#include <linux/list.h>

typedef const struct {
	char *name;
/*cmd_xxx_func(int argc, char *argv) note:
	1, return code: 0 -> finished success, <0 finished with err, >0 repeat exec needed
	2, in repeat exec mode, argc = 0;
*/
	int (*func)(int argc, char *argv[]);
	char *help;
} cmd_t;

struct cmd_list_s {
	char *cmdline;
	short len;
	short ms; //repeat period
	time_t deadline;
	struct list_head list;
};

struct cmd_queue_s {
	int flag;
	cmd_t *trap;
	struct list_head cmd_list;
};

#define CMD_FLAG_REPEAT 1

#pragma section=".shell.cmd" 4
#define DECLARE_SHELL_CMD(cmd) \
	const cmd_t *##cmd##_entry@".shell.cmd" = &##cmd;
	
/*cmd module i/f*/
void cmd_Init(void);
void cmd_Update(void);

/*cmd queue ops*/
int cmd_queue_init(struct cmd_queue_s *);
int cmd_queue_update(struct cmd_queue_s *);
int cmd_queue_exec(struct cmd_queue_s *, const char *);

/*common command related subroutines*/
int cmd_pattern_get(const char *str); //get pattern from a string, such as: "0,2-5,8" or "all,1" for inverse selection

#endif /*__CMD_H_*/
