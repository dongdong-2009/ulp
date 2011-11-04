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
#include "debug.h"
#include "uart.h"

static void cmd_GetHistory(char *cmd, int dir);
static void cmd_SetHistory(const char *cmd);

static LIST_HEAD(shell_queue);
static struct shell_s *shell; /*current shell*/

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
}

void shell_Update(void)
{
	int ok;
	struct list_head *pos;
	list_for_each(pos, &shell_queue) {
		shell = list_entry(pos, shell_s, list);
		console_select(shell -> console);
		cmd_queue_update(&shell -> cmd_queue);

		ok = shell_ReadLine(CONFIG_SHELL_PROMPT, NULL);
		if(!ok) continue;
		cmd_queue_exec(&shell -> cmd_queue, shell -> cmd_buffer);
	}
}

DECLARE_LIB(shell_Init, shell_Update)

int shell_register(const struct console_s *console)
{
	shell = sys_malloc(sizeof(struct shell_s));
	assert(shell != NULL);

	//sponsor new shell
	list_add(&shell -> list, &shell_queue);
	shell -> console = console;
	shell -> cmd_buffer[0] = 0;
	shell -> cmd_idx = -1;

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

	//new shell init
	console_select(shell -> console);
	cmd_queue_init(&shell -> cmd_queue);
#ifdef stdout
	setbuf(stdout, 0);
	setbuf(stderr, 0);
#endif
	putchar(0x1b); /*clear screen*/
	putchar('c');
	printf("%s\n", CONFIG_BLDC_BANNER);
	return 0;
}

int shell_exec_cmd(const struct console_s *cnsl, const char *cmdline)
{
	struct list_head *pos;
	list_for_each(pos, &shell_queue) {
		shell = list_entry(pos, shell_s, list);
		if(shell -> console == cnsl) {
			console_select(cnsl);
			cmd_queue_exec(&shell -> cmd_queue, cmdline);
			return 0;
		}
	}

	return -1;
}

/*read a line of string from current console*/
int shell_ReadLine(const char *prompt, char *str)
{
	int ch, len, sz, offset, tmp, idx, carry_flag;
	char buf[CONFIG_SHELL_LEN_CMD_MAX];
	int ready = 0;

	assert(shell != NULL);
	if(shell -> cmd_idx < 0) {
		if(prompt != NULL) {
			printf("%s", prompt);
		}
		memset(shell -> cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
		shell -> cmd_idx ++;
		shell -> cmd_hrpos = shell -> cmd_hrail - 1;
		if(shell -> cmd_hrpos < 0)
			shell -> cmd_hrpos = shell -> cmd_hsz;
	}

	if(shell -> cmd_idx <= -1) { /*+/- key for quick debug purpose*/
		shell -> cmd_idx --;
		cmd_GetHistory(shell -> cmd_buffer, -1);
		shell -> cmd_idx = -2 - shell -> cmd_idx;

		/*terminal display*/
		printf(shell -> cmd_buffer);
		offset = strlen(shell -> cmd_buffer) - shell -> cmd_idx;
		if(offset > 0)
			printf("\033[%dD", offset); /*restore cursor position*/
	}

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
		case '@':
			putchar('\r');
		case '\r':		// Return
			shell -> cmd_idx = -1;
			ready = 1;
			putchar('\n');
			break;
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
				putchar(127);
				printf("\033[s"); /*save cursor pos*/
				printf("\033[K"); /*clear contents after cursor*/
				printf(buf);
				printf("\033[u"); /*restore cursor pos*/
			}
			continue;
		case 0x1B: /*arrow keys*/
			ch = console_getch();
			if(ch != '[')
				continue;
			ch = console_getch();
			switch (ch) {
				case 'A': /*UP key*/
					offset = shell -> cmd_idx;
					ch = shell->cmd_buffer[offset];
					if((offset == len) /*|| ((offset < len) && ((ch < '0') || (ch > '9')))*/) {
						memset(shell -> cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
						cmd_GetHistory(shell -> cmd_buffer, -1);
						shell -> cmd_idx = strlen(shell -> cmd_buffer);

						/*terminal display*/
						if(offset > 0)
							printf("\033[%dD", offset); /*mov cursor to left*/
						printf("\033[K"); /*clear contents after cursor*/
						printf(shell -> cmd_buffer);
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
							printf("\033[%dD", offset); /*mov cursor to left*/
						printf("\033[K"); /*clear contents after cursor*/
						printf(shell -> cmd_buffer);
					}
					else
						ch = '-';
					break;
				case 'C': /*RIGHT key*/
					if(shell -> cmd_idx < len) {
						shell -> cmd_idx ++;
						printf("\033[C"); /*mov cursor right*/
					}
					break;
				case 'D': /*LEFT key*/
					if(shell -> cmd_idx > 0) {
						shell -> cmd_idx --;
						printf("\033[D"); /*mov cursor left*/
					}
					break;
				default:
					break;
			}
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
							printf("\033[D"); /*left shift 1 char*/
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
					printf("\033[1C"); /*right shift 1 char*/
					putchar(127);
					putchar(tmp);
					printf("\033[D"); /*left shift 1 char*/
				} while(carry_flag);

				if(idx == shell -> cmd_idx)
					continue;

				shell -> cmd_idx = -2 - shell -> cmd_idx;
				ready = 1;
				putchar('\n');
			}
			continue;
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
				putchar(ch);
				if(offset < len) {
					printf("\033[s"); /*save cursor pos*/
					printf("\033[K"); /*clear contents after cursor*/
					printf(buf);
					printf("\033[u"); /*restore cursor pos*/
				}
			}
			continue;
		}
	}

	if(ready) {
		if(str != NULL) {
			strcpy(str, shell -> cmd_buffer);
		}

		if(strlen(shell -> cmd_buffer))
			cmd_SetHistory(shell -> cmd_buffer);
	}
	return ready;
}

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

