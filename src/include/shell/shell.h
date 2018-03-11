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


enum shell_status {
	SHELL_STATUS_INIT,
	SHELL_STATUS_RUN,
};

struct shell_s {
	const struct console_s *console;

	short status;
	#define SHELL_CONFIG_MUTE (1<<0)
	#define SHELL_CONFIG_LOCK (1<<1)
	short config;

	/*cmd line*/
	const char *prompt;
	char cmd_buffer[CONFIG_SHELL_LEN_CMD_MAX];
	short cmd_idx;

#if CONFIG_SHELL_LEN_HIS_MAX > 0
	/*cmd history*/
	short cmd_hsz; /*cmd history size*/
	char *cmd_history;
	short cmd_hrail;
	short cmd_hrpos;
#endif
#ifdef CONFIG_SHELL_MULTI
	struct list_head list; //list of shells
#endif
	struct cmd_queue_s cmd_queue;
};

void shell_Init(void);
void shell_Update(void);

/*to dynamic register a new shell console device with specified history buffer size*/
int shell_register(const struct console_s *);
int shell_unregister(const struct console_s *);
int shell_mute_set(const struct console_s *cnsl, int enable); //disable shell echo
#define shell_mute(cnsl) shell_mute_set(cnsl, 1)
#define shell_unmute(cnsl) shell_mute_set(cnsl, 0)
int shell_lock_set(const struct console_s *cnsl, int enable); //disable shell input
#define shell_lock(cnsl) shell_lock_set(cnsl, 1)
#define shell_unlock(cnsl) shell_lock_set(cnsl, 0)
int shell_trap(const struct console_s *cnsl, cmd_t *cmd);
int shell_prompt(const struct console_s *cnsl, const char *prompt);

/*to execute a specified cmd in specified console*/
int shell_exec_cmd(const struct console_s *, const char *cmdline);

/*read a line from current console, return 0 if not finish*/
int shell_ReadLine(const char *prompt, char *str);

#endif /*__SHELL_H_*/
