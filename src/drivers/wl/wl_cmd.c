#include "shell/shell.h"
#include "shell/cmd.h"
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ulp/wl.h"
#include "nvm.h"
#include "sys/sys.h"
#include "ulp/device.h"
#include "spi.h"
#include "ulp_time.h"

static unsigned wl_freq __nvm;
static unsigned wl_mode __nvm;
static unsigned wl_addr __nvm;

static int nrf_onfail(int ecode, ...)
{
	if(ecode != WL_ERR_RX_FRAME) //to avoid err trig in nrf_speed
		printf("ecode = %d\n", ecode);
	//assert(0); //!!! impossible: got rx_dr but no payload?
	return 0;
}

static int cmd_nrf_chat(void)
{
	int ready, n, newdat;
	char *rxstr, *txstr;

	rxstr = sys_malloc(128);
	txstr = sys_malloc(128);

	int fd = dev_open("wl0", 0);
	assert(fd != 0);
	dev_ioctl(fd, WL_SET_FREQ, wl_freq);
	dev_ioctl(fd, WL_SET_ADDR, wl_addr);
	dev_ioctl(fd, WL_SET_MODE, wl_mode);
	dev_ioctl(fd, WL_ERR_TXMS, 500);
	dev_ioctl(fd, WL_ERR_FUNC, nrf_onfail);
	dev_ioctl(fd, WL_START);

	printf("if it doesn't works, pls check the nrf work mode: prx = %d?\n", wl_mode);
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

	dev_ioctl(fd, WL_STOP);
	dev_close(fd);
	sys_free(txstr);
	sys_free(rxstr);
	return 0;
}

static int nrf_bytes_rx = 0;
static int nrf_bytes_tx = 0;
static int nrf_bytes_lost = 0;
static unsigned char nrf_byte_tx = 0; //byte been sent
static unsigned char nrf_byte_rx = 0; //byte been recv
static time_t wl_timer;
static int nrf_bytes_ts; //bytes tx in 1s
static int nrf_bytes_rs; //bytes rx in 1s

static int cmd_nrf_speed(void)
{
	int fd = dev_open("wl0", 0);
	assert(fd != 0);
	dev_ioctl(fd, WL_SET_FREQ, wl_freq);
	dev_ioctl(fd, WL_SET_ADDR, wl_addr);
	dev_ioctl(fd, WL_SET_MODE, wl_mode);
	dev_ioctl(fd, WL_ERR_TXMS, 500);
	dev_ioctl(fd, WL_ERR_FUNC, nrf_onfail);
	dev_ioctl(fd, WL_START);

	printf("if it doesn't works, pls check the nrf work mode: prx = %d?\n", wl_mode);
	printf("pls press any key to exit ...\n");

	//init
	nrf_bytes_tx = 0;
	nrf_bytes_rx = 0;
	nrf_bytes_lost = 0;
	nrf_byte_tx = 0;
	nrf_byte_rx = 0;

	unsigned char *buf = sys_malloc(128);
	unsigned char byte_rx = 0;
	int bytes_lost, ms, tx_bps, rx_bps, i;
	wl_timer = time_get(0);
	nrf_bytes_ts = 0;
	nrf_bytes_rs = 0;
	do {
		//try to recv 128 bytes
		if(dev_poll(fd, POLLIN) >= 128) {
			dev_read(fd, buf, 128);
			for(i = 0; i < 128; i ++) {
				byte_rx = buf[i];
				if(nrf_bytes_rx > 0) {
					bytes_lost = byte_rx - nrf_byte_rx; //255 -> 0
					bytes_lost += (bytes_lost < 0) ? 256 : 0;
					bytes_lost -= 1; //normal increase 1
					nrf_bytes_lost += bytes_lost;
					if(bytes_lost != 0) {
						printf("\n");
					}
				}
				nrf_bytes_rx ++;
				nrf_byte_rx = byte_rx;
			}
		}

		//try to send a byte
		if(dev_poll(fd, POLLOBUF) >= 128) {
			for(i = 0; i < 128; i ++) {
				nrf_bytes_tx ++;
				nrf_byte_tx ++;
				buf[i] = nrf_byte_tx;
			}
			dev_write(fd, buf, 128);
		}

		ms = -time_left(wl_timer);
		ms = (ms == 0) ? 1 : ms;
		if(ms > 1000) {
			//update speed display
			tx_bps = (nrf_bytes_tx - nrf_bytes_ts) * 1000 / ms;
			rx_bps = (nrf_bytes_rx - nrf_bytes_rs) * 1000 / ms;
			printf("\rnrf: tx %dB(%dBPS) rx %dB(%dBPS) lost %dB        ", nrf_bytes_tx, tx_bps, nrf_bytes_rx, rx_bps, nrf_bytes_lost);

			//next speed calcu period
			wl_timer = time_get(0);
			nrf_bytes_ts = nrf_bytes_tx;
			nrf_bytes_rs = nrf_bytes_rx;
		}
	} while(!console_IsNotEmpty());
	printf("\n");
	dev_ioctl(fd, WL_STOP);
	sys_free(buf);
	dev_close(fd);
	return 0;
}

static int cmd_nrf_func(int argc, char *argv[])
{
	int show_help = 1;
	const char *usage = {
		"usage:\n"
		"nrf chat		start nrf chat small test program\n"
		"nrf speed		nrf speed test\n"
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

		if(!strcmp(argv[1], "speed")) {
			return cmd_nrf_speed();
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
