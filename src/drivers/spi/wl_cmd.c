#include "shell/shell.h"
#include "shell/cmd.h"
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "wl.h"
#include "nvm.h"
#include "sys/sys.h"

static wl_cfg_t nrf_cfg __nvm; /*only for debug cmd use*/

static int cmd_nrf_chat(void)
{
	int ready, n;
	char *rxstr, *txstr;
	
	rxstr = sys_malloc(128);
	txstr = sys_malloc(128);
	
	wl0.init(&nrf_cfg);
	wl0.select(0x12345678);
	while(1) {
		//tx
		ready = shell_ReadLine("nrf tx: ", txstr);
		if(ready) {
			if(!strncmp(txstr, "kill", 4)) {
				break;
			}
			
			n = wl0.send(txstr, strlen(txstr) + 1, 5);
			if(n != strlen(txstr) + 1) {
				printf("...fail\n");
			}
		}
		
		//rx
		ready = wl0.poll();
		if(ready) {
			n = wl0.recv(rxstr, 128, 5);
			if(n > 0) {
				rxstr[n] = 0;
				printf("\rnrf rx: %s\nnrf tx: ", rxstr); 
			}
		}
	}
	
	return 0;
}

static int cmd_nrf_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"nrf chat	start nrf chat\n"
		"nrf ch 0-127	set RF channel(2400MHz + ch * 1Mhz)\n"
	};
	
	if(argc > 1) {
		if(!strcmp(argv[1], "ch")) {
			int ch = (argc > 2) ? atoi(argv[2]) : 0;
			ch = ((ch >= 0) && (ch <= 127)) ? ch : 0;
			nrf_cfg.mhz = 24000 + ch;
			printf("nrf: setting RF ch = %d(%dMHz)\n", ch, nrf_cfg.mhz);
			nvm_save();
			return 0;
		}
		if(!strcmp(argv[1], "chat")) {
			return cmd_nrf_chat();
		}
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_nrf = {"nrf", cmd_nrf_func, "nrf debug command"};
DECLARE_SHELL_CMD(cmd_nrf)
