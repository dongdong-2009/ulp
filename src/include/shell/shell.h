/*
 * 	miaofng@2009 initial version
 */
#ifndef __SHELL_H_
#define __SHELL_H_

#include "config.h"
#include "console.h"
#include "shell/cmd.h"
#include "linux/list.h"

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

#ifndef CONFIG_SHELL_LEN_HIS_MAX
#define CONFIG_SHELL_LEN_HIS_MAX 64
#endif

struct shell_s {
	const struct console_s *console;

	short status;
	#define SHELL_CONFIG_MUTE (1<<0)
	#define SHELL_CONFIG_LOCK (1<<1)
	short config;

	/*cmd line*/
	char cmd_buffer[CONFIG_SHELL_LEN_CMD_MAX];
	short cmd_idx;

	/*cmd history*/
	short cmd_hsz; /*cmd history size*/
	char *cmd_history;
	short cmd_hrail;
	short cmd_hrpos;

	struct list_head list; //list of shells
	struct cmd_queue_s cmd_queue;
};

void shell_Init(void);
void shell_Update(void);

/*to dynamic register a new shell console device with specified history buffer size*/
int shell_register(const struct console_s *);
int shell_unregister(const struct console_s *);
int shell_mute(const struct console_s *cnsl, int enable); //disable shell echo
int shell_lock(const struct console_s *cnsl, int enable); //disable shell input
int shell_trap(const struct console_s *cnsl, cmd_t *cmd);

/*to execute a specified cmd in specified console*/
int shell_exec_cmd(const struct console_s *, const char *cmdline);

/*read a line from current console, return 0 if not finish*/
int shell_ReadLine(const char *prompt, char *str);

#endif /*__SHELL_H_*/
