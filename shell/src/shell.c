/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "cmd.h"

static int shell_ReadLine(void);
static void shell_Parse(void);
static void cmd_GetHistory(char *cmd, int dir);
static void cmd_SetHistory(const char *cmd);

static char cmd_history[CONFIG_SHELL_LEN_HIS_MAX];
static int cmd_hrail;
static int cmd_hrpos;
static char cmd_buffer[CONFIG_SHELL_LEN_CMD_MAX]; /*max length of a cmd and paras*/
static int cmd_idx;
static char *argv[CONFIG_SHELL_NR_PARA_MAX]; /*max number of para, include command itself*/
static int argc;

void shell_Init(void)
{
	cmd_buffer[0] = 0;
	cmd_SetHistory("help");
	cmd_idx = -1;
	
	putchar(0x1b); /*clear screen*/
	putchar('c');
	
	cmd_Init();
	printf("%s\n", CONFIG_BLDC_BANNER);
}

void shell_Update(void)
{
	int ok;
	
	cmd_Update();
	ok = shell_ReadLine();
	if(!ok) return;

	shell_Parse();
	cmd_Exec(argc, argv);
}

/*read a line of string from console*/
static int shell_ReadLine(void)
{
	int ch, len, sz, offset, tmp;
	char buf[CONFIG_SHELL_LEN_CMD_MAX];
	int ready = 0;
	
	if(cmd_idx < 0) { 
		printf("%s", CONFIG_SHELL_PROMPT);
		memset(cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
		cmd_idx ++;
		cmd_hrpos = cmd_hrail - 1;
		if(cmd_hrpos < 0)
			cmd_hrpos = CONFIG_SHELL_LEN_HIS_MAX;
	}

	if(cmd_idx <= -1) { /*+/- key for quick debug purpose*/
		cmd_idx --;
		cmd_GetHistory(cmd_buffer, -1);
		cmd_idx = -2 - cmd_idx;
				
		/*terminal display*/
		printf(cmd_buffer);
		offset = strlen(cmd_buffer) - cmd_idx;
		if(offset > 0)
			printf("\033[%dD", offset); /*restore cursor position*/
	}

	len = strlen(cmd_buffer);
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
				strcpy(cmd_buffer, "pause");
			else
				strcpy(cmd_buffer, "kill all");
		case '\r':		// Return
			cmd_idx = -1;
			ready = 1;
			putchar('\n');
			break;
		case '+':
		case '-':
			tmp = cmd_buffer[cmd_idx];
			if( tmp < '0' || tmp > '9')
				continue;

			if(ch == '+') {
				tmp ++;
				if(tmp > '9') 
					tmp = '0';
			}
			else {
				tmp --;
				if(tmp < '0')
					tmp = '9';
			}

			/*replace*/
			cmd_buffer[cmd_idx] = tmp;

			/*terminal display*/
			printf("\033[1C"); /*right shift 1 char*/
			putchar(127);
			putchar(tmp);
			
			cmd_idx = -2 - cmd_idx;
			ready = 1;
			putchar('\n');
			break;
		case 127:			// Backspace
			if(cmd_idx > 0)
			{
				sz = len - cmd_idx;
				if(sz > 0) {
					/*copy cursor->rail to buf*/
					offset = cmd_idx;
					memcpy(buf, cmd_buffer + offset, sz);
					/*copy back*/
					offset = cmd_idx - 1;
					memcpy(cmd_buffer + offset, buf, sz);
				}

				cmd_buffer[len - 1] = 0;
				cmd_idx --;

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
					offset = cmd_idx;
					memset(cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
					cmd_GetHistory(cmd_buffer, -1);
					cmd_idx = strlen(cmd_buffer);
					
					/*terminal display*/
					if(offset > 0)
						printf("\033[%dD", offset); /*mov cursor to left*/
					printf("\033[K"); /*clear contents after cursor*/
					printf(cmd_buffer);
					break;
				case 'B': /*DOWN key*/
					offset = cmd_idx;
					memset(cmd_buffer, 0, CONFIG_SHELL_LEN_CMD_MAX);
					cmd_GetHistory(cmd_buffer, 1);
					cmd_idx = strlen(cmd_buffer);
					
					/*terminal display*/
					if(offset > 0)
						printf("\033[%dD", offset); /*mov cursor to left*/
					printf("\033[K"); /*clear contents after cursor*/
					printf(cmd_buffer);
					break;
				case 'C': /*RIGHT key*/
					if(cmd_idx < len) {
						cmd_idx ++;
						printf("\033[C"); /*mov cursor right*/
					}
					break;
				case 'D': /*LEFT key*/
					if(cmd_idx > 0) {
						cmd_idx --;
						printf("\033[D"); /*mov cursor left*/
					}
					break;
				default:
					break;
			}
			continue;
		case '/':
			ch = '-';
		default:
			if((ch < ' ') || (ch > 126))
				continue;
			if(len < CONFIG_SHELL_LEN_CMD_MAX - 1)
			{
				sz = len - cmd_idx;
				if(sz > 0) {
					/*copy cursor->rail to buf*/
					offset = cmd_idx;
					memcpy(buf, cmd_buffer + offset, sz);
					
					/*copy back*/
					offset ++;
					memcpy(cmd_buffer + offset, buf, sz);
				}

				cmd_buffer[cmd_idx] = ch;
				cmd_idx ++;
				
				/*terminal display*/
				putchar(ch);
				printf("\033[s"); /*save cursor pos*/
				printf("\033[K"); /*clear contents after cursor*/
				printf(buf);
				printf("\033[u"); /*restore cursor pos*/
			}
			continue;
		}
	}

	if(ready) {
		if(strlen(cmd_buffer))
			cmd_SetHistory(cmd_buffer);
	}
	return ready;
}

static void shell_Parse(void)
{
	int len, i = 0;
	char ch;
	int flag = 0;
	
	argc = 0;
	len = strlen(cmd_buffer);
	ch = cmd_buffer[i];
	while(ch) {
		if(ch != ' ') {
			if(argc > CONFIG_SHELL_NR_PARA_MAX)
				break;
			
			if(flag == 0) {
				argv[argc] = cmd_buffer + i;
				argc ++;
				flag = 1;
			}
		}
		else {
			cmd_buffer[i] = 0;
			flag = 0;
		}
			
		i++;
		
		if(i < len)
			ch = cmd_buffer[i];
		else
			break;
	}
	
	/*debug*/
#if 0
	for(i = 0; i < argc; i++)
		printf("argv[%d] = %s\n", i, argv[i]);
#endif
}

/*cmd history format: d0cmd0cmd0000000cm*/
static void cmd_GetHistory(char *cmd, int dir)
{
	char ch;
	int ofs, len, cnt, bytes;

	bytes = 0;
	dir = (dir > 0) ? 1 : -1;
	
	/*search next cmd related to current offset*/
	ofs = cmd_hrpos;
	
	for(cnt = 0; cnt < CONFIG_SHELL_LEN_HIS_MAX; cnt ++) {
		ch = cmd_history[ofs];
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
		if( ofs >= CONFIG_SHELL_LEN_HIS_MAX)
			ofs -= CONFIG_SHELL_LEN_HIS_MAX;
		else if(ofs < 0)
			ofs = CONFIG_SHELL_LEN_HIS_MAX - 1;
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
	
	cmd_hrpos = ofs;
}

static void cmd_SetHistory(const char *cmd)
{
	int ofs, len, cnt;
	
	/*insert the cmd to rail*/
	ofs = cmd_hrail;
	len = strlen(cmd);
	if( !len)
		return;
	
	for(cnt = 0; cnt < CONFIG_SHELL_LEN_HIS_MAX; cnt ++) {
		if(cnt < len) {
			cmd_history[ofs] = cmd[cnt];
			cmd_hrail = ofs + 2;
		}
		else {
			if(cmd_history[ofs] == 0)
				break;
			cmd_history[ofs] = 0;
		}
		
		ofs ++;
		if( ofs >= CONFIG_SHELL_LEN_HIS_MAX)
			ofs -= CONFIG_SHELL_LEN_HIS_MAX;
	}

	if( cmd_hrail >= CONFIG_SHELL_LEN_HIS_MAX)
		cmd_hrail -= CONFIG_SHELL_LEN_HIS_MAX;
}

