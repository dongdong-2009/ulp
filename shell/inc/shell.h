/*
 * 	miaofng@2009 initial version
 */
#ifndef __SHELL_H_
#define __SHELL_H_

#ifndef CONFIG_SHELL_PROMPT
#define CONFIG_SHELL_PROMPT "bldc# "
#endif

#ifndef CONFIG_BLDC_BANNER
#define CONFIG_BLDC_BANNER "Welcome to Brushless Motor Console"
#endif

#ifndef CONFIG_SHELL_LEN_CMD_MAX
#define CONFIG_SHELL_LEN_CMD_MAX 32
#endif

#ifndef CONFIG_SHELL_NR_PARA_MAX
#define CONFIG_SHELL_NR_PARA_MAX 8
#endif

typedef struct {
	char *name;
	int (*cmd)(int argc, char *argv[]);
	char *help;
} cmd_t;

typedef struct {
	cmd_t *cmd;
	void *next;
} cmd_list_t;

void shell_Init(void);
void shell_Update(void);
void shell_AddCmd(cmd_t *cmd);

#endif /*__SHELL_H_*/
