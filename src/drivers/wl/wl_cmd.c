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
#include "ulp/device.h"
#include "spi.h"

static unsigned wl_freq __nvm;
static unsigned wl_mode __nvm;
static unsigned wl_addr __nvm;

const struct nrf_cfg_s nrf_cfg = {
	.spi = &spi1,
	.gpio_cs = SPI_1_NSS,
	.gpio_ce = SPI_CS_PC5,
};

static int cmd_nrf_chat(void)
{
	int ready, n, newdat;
	char *rxstr, *txstr;

	rxstr = sys_malloc(128);
	txstr = sys_malloc(128);

	dev_register("nrf", &nrf_cfg);
	int fd = dev_open("wl0", 0);
	assert(fd != 0);
	dev_ioctl(fd, WL_SET_FREQ, wl_freq);
	dev_ioctl(fd, WL_SET_ADDR, wl_addr);
	dev_ioctl(fd, WL_SET_MODE, wl_mode | WL_MODE_1MBPS);

	printf("if it doesn't works, pls check the nrf work mode: ptx <> prx\n");
	printf("pls type kill to exit ...\n");
	while(1) {
		//tx
		ready = shell_ReadLine("nrf tx: ", txstr);
		if(ready) {
			if(!strncmp(txstr, "kill", 4)) {
				break;
			}

			n = dev_write(fd, txstr, strlen(txstr) + 1);
			if(n != strlen(txstr) + 1) {
				printf("...fail\n");
			}
		}

		//rx
		newdat = 0;
		while((n = dev_poll(fd, POLLIN)) > 0) {
			n = (n > 127) ? 127 : n;
			n = dev_read(fd, rxstr, n);
			if(n > 0) {
				rxstr[n] = 0;
				if(newdat == 0)
					printf("\rnrf rx: ");
				printf("%s", rxstr);
				newdat = 1;
			}
		}
		if(newdat)
			printf("\nnrf tx: ");
	}

	sys_free(txstr);
	sys_free(rxstr);
	return 0;
}

static int cmd_nrf_func(int argc, char *argv[])
{
	int show_help = 1;
	const char *usage = {
		"usage:\n"
		"nrf chat		start nrf chat small test program\n"
		"nrf freq 2400		set RF channel(2400MHz + (0-127) * 1Mhz)\n"
		"nrf mode ptx?prx	set nrf work mode\n"
		"nrf addr 0x12345678	set nrf phy addr, hex\n"
	};

	if(argc > 1) {
		show_help = 0;
		if(!strcmp(argv[1], "freq")) {
			int ch;
			if(argc > 2) {
				ch = atoi(argv[2]);
				ch -= 2400;
				ch = ((ch >= 0) && (ch <= 127)) ? ch : 0;
				wl_freq = 2400 + ch;
				nvm_save();
			}
		}

		if(!strcmp(argv[1], "mode")) {
			if(argc > 2) {
				wl_mode = 0;
				if(!strcmp(argv[2], "prx")) {
					wl_mode = WL_MODE_PRX;
				}
				nvm_save();
			}
		}

		if(!strcmp(argv[1], "addr")) {
			if(argc > 2) {
				sscanf(argv[2], "0x%x", &wl_addr);
				nvm_save();
			}
		}

		if(!strcmp(argv[1], "chat")) {
			return cmd_nrf_chat();
		}
	}

	/*printf general settings*/
	char *mode = (wl_mode & WL_MODE_PRX ) ? "prx" : "ptx";
	printf("nrf: mode = %s\n", mode);
	printf("nrf: freq = %dMHz\n", wl_freq);
	printf("nrf: addr = 0x%08x\n", wl_addr);

	if(show_help)
		printf("%s", usage);
	return 0;
}

const cmd_t cmd_nrf = {"nrf", cmd_nrf_func, "nrf debug command"};
DECLARE_SHELL_CMD(cmd_nrf)
