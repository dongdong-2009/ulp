/*
 * 	miaofng@2009 initial version
 */
#ifndef __CMD_H_
#define __CMD_H_

typedef struct {
	char *name;
	int (*func)(int argc, char *argv[]);
	char *help;
} cmd_t;

typedef struct {
	cmd_t *cmd;
	int flag;
	void *next;
} cmd_list_t;

#define CMD_FLAG_REPEAT 1

#pragma section=".shell.cmd" 4
#define DECLARE_SHELL_CMD(cmd) \
	const cmd_t *##cmd##_entry@".shell.cmd" = &##cmd;

void cmd_Init(void);
void cmd_Update(void);
void cmd_Add(cmd_t *cmd);
void cmd_Exec(int argc, char *argv[]);

#endif /*__SHELL_H_*/
