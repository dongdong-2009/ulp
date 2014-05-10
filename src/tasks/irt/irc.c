/*
*
*  miaofng@2014-5-10   initial version
*
*/

#include "ulp/sys.h"
#include "irc.h"
#include "vm.h"
#include "nvm.h"
#include "led.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"

int main(void)
{
	sys_init();
	vm_init();
	printf("irc v1.0, SW: %s %s\n\r", __DATE__, __TIME__);
	while(1) {
	}
}

#include "shell/cmd.h"
#if 1
static int cmd_irc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
	};

	int e;
	for(int i = 1; i < argc ; i ++) {
		e = (argv[i][0] != '-');
		if(!e) {
			switch(argv[i][1]) {
			case 'i':
				break;
			default:
				break;
			}
		}
	}
	if(e) {
		printf("%s", usage);
	}
	return 0;
}

const cmd_t cmd_irc = {"irc", cmd_irc_func, "irc debug commands"};
DECLARE_SHELL_CMD(cmd_irc)
#endif
