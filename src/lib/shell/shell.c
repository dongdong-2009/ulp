/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "shell/shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/task.h"
#include "sys/sys.h"
#include "shell/cmd.h"
#include "console.h"
#include "ulp/debug.h"
#include "uart.h"
#include <stdarg.h>

#ifdef CONFIG_CMD_AUTORUN
#include "nvm.h"
static char shell_autorun_cmdline[CONFIG_SHELL_LEN_CMD_MAX] __nvm;
#endif

#if CONFIG_SHELL_LEN_HIS_MAX > 0
static void cmd_GetHistory(char *cmd, int dir);
static void cmd_SetHistory(const char *cmd);
#endif

static const char *shell_prompt_def = CONFIG_SHELL_PROMPT;
#ifdef CONFIG_SHELL_MULTI
static LIST_HEAD(shell_queue);
#endif
static struct shell_s *shell = NULL; /*current shell*/

#define shell_print(...) do { \
	if(!(shell->config & SHELL_CONFIG_MUTE)) \
		printf(__VA_ARGS__); \
} while (0)

void shell_Init(void)
{
	uart_cfg_t cfg = UART_CFG_DEF;
	cfg.baud = BAUD_115200;
#ifdef CONFIG_CONSOLE_BAUD
	cfg.baud = CONFIG_CONSOLE_BAUD;
#endif

	cmd_Init();
#ifdef CONFIG_SHELL_UARTg
	uartg.init(&cfg);
	shell_register((const struct console_s *) &uartg);
#endif
#ifdef CONFIG_SHELL_UART0
	uart0.init(&cfg);
	shell_register((const struct console_s *) &uart0);
#endif
#ifdef CONFIG_SHELL_UART1
	uart1.init(&cfg);
	shell_register((const struct console_s *) &uart1);
#endif
#ifdef CONFIG_SHELL_UART2
	uart2.init(&cfg);
	shell_register((const struct console_s *) &uart2);
#endif
#ifdef CONFIG_SHELL_UART3
	uart3.init(&cfg);
	shell_register((const struct console_s *) &uart3);
#endif
#ifdef CONFIG_SHELL_UART4
	uart4.init(&cfg);
	shell_register((const struct console_s *) &uart4);
#endif
}

void shell_Update(void)
{
	int ok;
#ifdef CONFIG_SHELL_MULTI
	struct list_head *pos;
	list_for_each(pos, &shell_queue) {
		shell = list_entry(pos, shell_s, list);
#endif
		if((shell->config & SHELL_CONFIG_LOCK) == 0) {
			if(shell->status == SHELL_STATUS_INIT) { //first update
				shell->status = SHELL_STATUS_RUN;
				if(shell->prompt != NULL) {
					shell_print("%s", shell->prompt);
				}
			}
			console_set(shell -> console);
			cmd_queue_update(&shell -> cmd_queue);
			ok = shell_ReadLine(shell->prompt, NULL);
			if(ok) {
				cmd_queue_exec(&shell -> cmd_queue, shell -> cmd_buffer);
				if(shell->prompt != NULL) {
					shell_print("%s", shell->prompt);
				}
			}
		}
#ifdef CONFIG_SHELL_MULTI
		console_set(NULL); //restore system default console
	}
#endif
}

DECLARE_LIB(shell_Init, shell_Update)

int shell_register(const struct console_s *console)
{
	shell = sys_malloc(sizeof(struct shell_s));
	assert(shell != NULL);

#ifdef CONFIG_SHELL_MULTI
	//sponsor new shell
	list_add(&shell -> list, &shell_queue);
#endif
	shell -> console = console;
	shell -> status = SHELL_STATUS_INIT;
	shell -> config = 0;
	shell -> cmd_buffer[0] = 0;
	shell -> cmd_idx = -1;

	shell -> prompt = shell_prompt_def;
#if CONFIG_SHELL_LEN_HIS_MAX > 0
	shell -> cmd_hsz = CONFIG_SHELL_LEN_HIS_MAX;
	shell -> cmd_history = NULL;
	shell -> cmd_hrail = 0;
	shell -> cmd_hrpos = 0;
	if(shell -> cmd_hsz > 0) {
		shell -> cmd_history = sys_malloc(shell -> cmd_hsz);
		assert(shell -> cmd_history != NULL);
		memset(shell -> cmd_history, 0, shell -> cmd_hsz);
		cmd_SetHistory("help");
	}
#endif
	//new shell init
	console_set(shell -> console);
	cmd_queue_init(&shell -> cmd_queue);
#ifdef stdout
	setbuf(stdout, 0);
	setbuf(stderr, 0);
#endif
	shell_print("\033c"); /*clear screen*/
	shell_print("%s\n", CONFIG_BLDC_BANNER);
#ifdef CONFIG_SHELL_AUTORUN
	shell->cmd_buffer[0] = 0;
	if(strlen(CONFIG_AUTORUN_CMDLINE) > 0) {
		strcpy(shell->cmd_buffer, CONFIG_AUTORUN_CMDLINE);
	}
	#ifdef CONFIG_CMD_AUTORUN
	if(shell_autorun_cmdline[0] == 'A') {
		strcpy(shell->cmd_buffer, shell_autorun_cmdline + 1);
	}
	#endif
	cmd_queue_exec(&shell -> cmd_queue, shell -> cmd_buffer);
#endif
	console_set(NULL);
	return 0;
}

int shell_unregister(const struct console_s *console)
{
#ifdef CONFIG_SHELL_MULTI
	struct list_head *pos, *n;
	struct shell_s *q = NULL;

	list_for_each_safe(pos, n, &shell_queue) {
		q = list_entry(pos, shell_s, list);
		if(q->console == console) {
#if CONFIG_SHELL_LEN_HIS_MAX > 0
			//destroy cmd history
			sys_free(q->cmd_history);
#endif
			//destroy cmd list
			struct cmd_list_s *cl;
			list_for_each_safe(pos, n, &(q->cmd_queue.cmd_list)) {
				cl = list_entry(pos, cmd_list_s, list);
				list_del(&cl->list);
				sys_free(cl);
			}

			//destroy shell itself
			list_del(&q->list);
			sys_free(q);
			break;
		}
	}
#endif
	return 0;
}

static struct shell_s *shell_get(const struct console_s *cnsl)
{
#ifdef CONFIG_SHELL_MULTI
	struct shell_s *s;
	struct list_head *pos;
	list_for_each(pos, &shell_queue) {
		s = list_entry(pos, shell_s, list);
		if(s->console == cnsl) {
			return s;
		}
	}
#else
	if(shell != NULL) //only 1 cnsl, always return default shell
		return shell;
#endif
	return NULL;
}

int shell_mute_set(const struct console_s *cnsl, int enable)
{
	int ret = -1;
	struct shell_s *s;

#ifdef CONFIG_SHELL_MULTI
	if(cnsl == NULL) { //mute all console
		struct list_head *pos;
		list_for_each(pos, &shell_queue) {
			s = list_entry(pos, shell_s, list);
			if(enable == -1) {
				s->config ^= SHELL_CONFIG_MUTE;
			}
			else {
				enable = (enable) ? SHELL_CONFIG_MUTE : 0;
				s->config &= ~SHELL_CONFIG_MUTE;
				s->config |= enable;
			}
		}
		return 0;
	}
#endif

	s = shell_get(cnsl);
	if(s != NULL) {
		if(enable == -1) {
			s->config ^= SHELL_CONFIG_MUTE;
		}
		else {
			enable = (enable) ? SHELL_CONFIG_MUTE : 0;
			s->config &= ~SHELL_CONFIG_MUTE;
			s->config |= enable;
		}
		ret = 0;
	}
	return ret;
}

int shell_lock_set(const struct console_s *cnsl, int enable)
{
	int ret = -1;
	struct shell_s *s = shell_get(cnsl);
	if(s != NULL) {
		enable = (enable) ? SHELL_CONFIG_LOCK : 0;
		s->config &= ~SHELL_CONFIG_LOCK;
		s->config |= enable;
		ret = 0;
	}
	return ret;
}

int shell_trap(const struct console_s *cnsl, cmd_t *cmd)
{
	int ret = -1;
	struct shell_s *s = shell_get(cnsl);
	if(s != NULL) {
		s->cmd_queue.trap = cmd;
		ret = 0;
	}
	return ret;
}

int shell_prompt(const struct console_s *cnsl, const char *prompt)
{
	int ret = -1;
	struct shell_s *s = shell_get(cnsl);
	if(s != NULL) {
		s->prompt = prompt;
		ret = 0;
	}
	return ret;
}

int shell_exec_cmd(const struct console_s *cnsl, const char *cmdline)
{
	struct shell_s *s = shell_get(cnsl);
	if(s != NULL) {
		console_set(cnsl);
		cmd_queue_exec(&shell -> cmd_queue, cmdline);
		return 0;
	}

	return -1;
}

/*read a line of string from current console*/
int shell_ReadLine(const char *prompt, char *str)
{
	int ch, len, sz, offset;
	char buf[CONFIG_SHELL_LEN_CMD_MAX];
	int ready = 0;

	assert(shell != NULL);
	if(shell -> cmd_idx < 0) {
		/*
		if(prompt != NULL) {
			shell_print("%s", prompt);
		}
		*/
		memset(shell -> cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
		shell -> cmd_idx ++;
#if CONFIG_SHELL_LEN_HIS_MAX > 0
		shell -> cmd_hrpos = shell -> cmd_hrail - 1;
		if(shell -> cmd_hrpos < 0)
			shell -> cmd_hrpos = shell -> cmd_hsz;
#endif
	}
#if CONFIG_SHELL_LEN_HIS_MAX > 0
	int tmp, idx, carry_flag;
	if(shell -> cmd_idx <= -1) { /*+/- key for quick debug purpose*/
		shell -> cmd_idx --;
		cmd_GetHistory(shell -> cmd_buffer, -1);
		shell -> cmd_idx = -2 - shell -> cmd_idx;

		/*terminal display*/
		shell_print(shell -> cmd_buffer);
		offset = strlen(shell -> cmd_buffer) - shell -> cmd_idx;
		if(offset > 0)
			shell_print("\033[%dD", offset); /*restore cursor position*/
	}
#endif

	len = strlen(shell -> cmd_buffer);
	memset(buf, 0, CONFIG_SHELL_LEN_CMD_MAX);

	while(console_IsNotEmpty())
	{
		ch = console_getch();

		switch(ch) {
		case '\n':		// NewLine
			continue;
		case 24: /*ctrl-x*/
		case 3: /*ctrl-c*/
			if(ch == 3)
				strcpy(shell -> cmd_buffer, "pause");
			else
				strcpy(shell -> cmd_buffer, "kill all");
#if CONFIG_CMD_RET_OPT
		case '@':
			shell_print("%c", '\r');
#endif
		case '\r':		// Return
			shell -> cmd_idx = -1;
			ready = 1;
			shell_print("%c", '\n');
			break;
		case 8:
		case 127:			// Backspace
			if(shell -> cmd_idx > 0)
			{
				sz = len - shell -> cmd_idx;
				if(sz > 0) {
					/*copy cursor->rail to buf*/
					offset = shell -> cmd_idx;
					memcpy(buf, shell -> cmd_buffer + offset, sz);
					/*copy back*/
					offset = shell -> cmd_idx - 1;
					memcpy(shell -> cmd_buffer + offset, buf, sz);
				}

				shell -> cmd_buffer[len - 1] = 0;
				shell -> cmd_idx --;

				/*terminal display*/
				shell_print("%c", ch);
				shell_print("\033[s"); /*save cursor pos*/
				shell_print("\033[K"); /*clear contents after cursor*/
				shell_print(buf);
				shell_print("\033[u"); /*restore cursor pos*/
			}
			continue;
		case 0x1B: /*arrow keys*/
			ch = console_getch();
			if((ch == 'm') || (ch == 'M')) { //CTRL+m
				strcpy(shell -> cmd_buffer, "shell -x");
				shell -> cmd_idx = -1;
				ready = 1;
				shell_print("\n");
				break;
			}
			else if(ch != '[')
				continue;
			ch = console_getch();
			switch (ch) {
#if CONFIG_SHELL_LEN_HIS_MAX > 0
				case 'A': /*UP key*/
					offset = shell -> cmd_idx;
					ch = shell->cmd_buffer[offset];
					if((offset == len) /*|| ((offset < len) && ((ch < '0') || (ch > '9')))*/) {
						memset(shell -> cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
						cmd_GetHistory(shell -> cmd_buffer, -1);
						shell -> cmd_idx = strlen(shell -> cmd_buffer);

						/*terminal display*/
						if(offset > 0)
							shell_print("\033[%dD", offset); /*mov cursor to left*/
						shell_print("\033[K"); /*clear contents after cursor*/
						shell_print(shell -> cmd_buffer);
					}
					else
						ch = '+';
					break;
				case 'B': /*DOWN key*/
					offset = shell -> cmd_idx;
					ch = shell->cmd_buffer[offset];
					if((offset == len) /*|| ((offset < len) && ((ch < '0') || (ch > '9')))*/) {
						memset(shell -> cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
						cmd_GetHistory(shell -> cmd_buffer, 1);
						shell -> cmd_idx = strlen(shell -> cmd_buffer);

						/*terminal display*/
						if(offset > 0)
							shell_print("\033[%dD", offset); /*mov cursor to left*/
						shell_print("\033[K"); /*clear contents after cursor*/
						shell_print(shell -> cmd_buffer);
					}
					else
						ch = '-';
					break;
#endif
				case 'C': /*RIGHT key*/
					if(shell -> cmd_idx < len) {
						shell -> cmd_idx ++;
						shell_print("\033[C"); /*mov cursor right*/
					}
					break;
				case 'D': /*LEFT key*/
					if(shell -> cmd_idx > 0) {
						shell -> cmd_idx --;
						shell_print("\033[D"); /*mov cursor left*/
					}
					break;
				case 49: /*home key*/
					console_getch(); //'~'
					while(shell->cmd_idx > 0) {
						shell->cmd_idx --;
						shell_print("\033[D"); /*mov cursor left*/
					}
					break;
				case 52: /*end key*/
					console_getch(); //'~'
					while(shell -> cmd_idx < len) {
						shell -> cmd_idx ++;
						shell_print("\033[C"); /*mov cursor right*/
					}
					break;
				default:
					break;
			}
#if CONFIG_SHELL_LEN_HIS_MAX > 0
			if((ch == '+') || (ch == '-')) {
				idx = shell -> cmd_idx;
				do {
					carry_flag = 0;
					tmp = shell -> cmd_buffer[idx];
					if( tmp < '0' || tmp > '9') {
						if(tmp != '.')
							break;
						else if(idx != shell -> cmd_idx) {
							//float support
							idx --;
							shell_print("\033[D"); /*left shift 1 char*/
							carry_flag = 1;
							continue;
						}
					}

					if(ch == '+') {
						tmp ++;
						if(tmp > '9') {
							tmp = '0';
							carry_flag = 1;
						}
					}
					else {
						tmp --;
						if(tmp < '0') {
							tmp = '9';
							carry_flag = 1;
						}
					}

					/*replace*/
					shell -> cmd_buffer[idx] = tmp;
					idx --;

					/*terminal display*/
					shell_print("\033[1C"); /*right shift 1 char*/
					shell_print("%c", 8); //it works for both putty and hyterm
					shell_print("%c", tmp);
					shell_print("\033[D"); /*left shift 1 char*/
				} while(carry_flag);

				if(idx == shell -> cmd_idx)
					break;

				shell -> cmd_idx = -2 - shell -> cmd_idx;
				ready = 1;
				shell_print("%c", '\n');
			}
#endif
			break;
		default:
			if(((ch < ' ') || (ch > 126)) && (ch != '\t'))
				continue;
			offset = shell ->cmd_idx;
			if(len < CONFIG_SHELL_LEN_CMD_MAX - 1)
			{
				sz = len - shell -> cmd_idx;
				if(sz > 0) {
					/*copy cursor->rail to buf*/
					offset = shell -> cmd_idx;
					memcpy(buf, shell -> cmd_buffer + offset, sz);

					/*copy back*/
					offset ++;
					memcpy(shell -> cmd_buffer + offset, buf, sz);
				}

				shell -> cmd_buffer[shell -> cmd_idx] = ch;
				shell -> cmd_idx ++;

				/*terminal display*/
				shell_print("%c", ch);
				if(offset < len) {
					shell_print("\033[s"); /*save cursor pos*/
					shell_print("\033[K"); /*clear contents after cursor*/
					shell_print(buf);
					shell_print("\033[u"); /*restore cursor pos*/
				}
			}
			continue;
		}

		if(ready) {
			if(str != NULL) {
				strcpy(str, shell -> cmd_buffer);
			}
#if CONFIG_SHELL_LEN_HIS_MAX > 0
			if(strlen(shell -> cmd_buffer))
				cmd_SetHistory(shell -> cmd_buffer);
#endif

			break;
		}
	}

	return ready;
}

#if CONFIG_SHELL_LEN_HIS_MAX > 0
/*cmd history format: d0cmd0cmd0000000cm*/
static void cmd_GetHistory(char *cmd, int dir)
{
	char ch;
	int ofs, len, cnt, bytes;

	bytes = 0;
	dir = (dir > 0) ? 1 : -1;

	/*search next cmd related to current offset*/
	ofs = shell -> cmd_hrpos;

	for(cnt = 0; cnt < shell -> cmd_hsz; cnt ++) {
		ch = shell -> cmd_history[ofs];
		if(ch != 0) {
			cmd[bytes] = ch;
			bytes ++;
		}
		else {
			if(bytes != 0) {
				cmd[bytes] = 0;
				break;
			}
		}

		ofs += dir;
		if( ofs >= shell -> cmd_hsz)
			ofs -= shell -> cmd_hsz;
		else if(ofs < 0)
			ofs = shell -> cmd_hsz - 1;
	}

	if(bytes == 0)
		return;

	/*swap*/
	if(dir < 0) {
		len = bytes;
		bytes = bytes >> 1;
		for(cnt = 0; cnt < bytes; cnt ++) {
			ch = cmd[cnt];
			cmd[cnt] = cmd[len - cnt - 1];
			cmd[len - cnt -1] = ch;
		}
	}

	shell -> cmd_hrpos = ofs;
}

static void cmd_SetHistory(const char *cmd)
{
	int ofs, len, cnt;

	/*insert the cmd to rail*/
	ofs = shell -> cmd_hrail;
	len = strlen(cmd);
	if( !len)
		return;

	for(cnt = 0; cnt < shell -> cmd_hsz; cnt ++) {
		if(cnt < len) {
			shell -> cmd_history[ofs] = cmd[cnt];
			shell -> cmd_hrail = ofs + 2;
		}
		else {
			if(shell -> cmd_history[ofs] == 0)
				break;
			shell -> cmd_history[ofs] = 0;
		}

		ofs ++;
		if( ofs >= shell -> cmd_hsz)
			ofs -= shell -> cmd_hsz;
	}

	if( shell -> cmd_hrail >= shell -> cmd_hsz)
		shell -> cmd_hrail -= shell -> cmd_hsz;
}
#endif

#ifdef CONFIG_CMD_AUTORUN
static char shell_autorun_cmdline[] __nvm;
static int cmd_autorun_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"autorun		display current auto cmdline\n"
		"autorun -s [xyz]	set/clr new autorun cmdline\n"
	};

	int e = 0;
	for(int i = 1; i < argc; i ++) {
		e += (argv[i][0] != '-');
		switch(argv[i][1]) {
		case 's':
			i ++;
			shell_autorun_cmdline[0] = 0;
			if(i < argc) {
				shell_autorun_cmdline[0] = 'A';
				memcpy(shell_autorun_cmdline + 1, argv[i], strlen(argv[i]));
			}
			nvm_save();
			break;

		default:
			e ++;
		}
	}

	printf("autorun = ");
	if(shell_autorun_cmdline[0] == 'A') {
		printf("%s", shell_autorun_cmdline + 1);
	}
	printf("\n");

	if(e)
		printf("%s", usage);

	return 0;
}

const cmd_t cmd_autorun = {"autorun", cmd_autorun_func, "autorun a specified cmdline"};
DECLARE_SHELL_CMD(cmd_autorun)
#endif

static int cmd_shell_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"shell -m[/a]		switch shell to manual[/auto] mode\n"
	};

	int e = (argc > 1) ? 0 : -1;
	for(int i = 1; i < argc; i ++) {
		e = (argv[i][0] != '-');
		if(e) break;

		switch(argv[i][1]) {
		case 'a':
			shell_mute(shell->console);
			printf("<+0, No Error\n\r");
			break;
		case 'm':
			shell_unmute(shell->console);
			printf("<+0, No Error\n\r");
			break;
			
		case 'x':
			shell_mute_set(shell->console, -1);
			printf("<+0, mode switched\n\r");
			break;

		default:
			e ++;
		}
	}

	if(e)
		printf("%s", usage);
	return 0;
}

const cmd_t cmd_shell = {"shell", cmd_shell_func, "shell management commands"};
DECLARE_SHELL_CMD(cmd_shell)
