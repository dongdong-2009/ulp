/*
 * miaofng (C) Copyright 2010
 *
 * verified in following situation:
 * - big file, size > 500KB, pass
 * - stm32, with console dma enabled/disabled, pass
 *
 */
#include "config.h"
#include "err.h"
#include "ulp/debug.h"
#include "console.h"
#include "sys/sys.h"
#include "shell/cmd.h"
#include "common/kermit.h"
#include "common/circbuf.h"
#include "flash.h"
#include <stdio.h>
#include "ulp_time.h"

#define TIMEOUT (6000) //mS

static int load_kermit(int addr)
{

#define MASK 0xfff00000
#define ROM ((int)(load_kermit) & MASK) /*ROM_SIZE < 1MB*/
#define RAM ((int)(&addr) & MASK) /*RAM SIZE < 1MB*/

	circbuf_t ibuf, obuf, echo;
	kermit_t *k;
	int sz, ret, length, target, obsz;
	char c;
	time_t timeout = time_get(60000);

	//write to ram/flash/none?
	if((addr & MASK) == RAM) {
		target = RAM;
		obsz = ~ MASK;
		obuf.data = (char *) addr;
		obuf.totalsize = obsz;
		buf_flush(&obuf);
		printf("dnload file to ram: 0x%08x ...\n", addr);
	}
	else if((addr & MASK) == ROM) {
		target = ROM;
		obsz = FLASH_PAGE_SZ;
		if(addr & (obsz - 1)) {
			printf("abort, flash addr must page aligned\n");
			return 0;
		}
		buf_init(&obuf, obsz);
		printf("dnload file to rom: 0x%08x ...\n", addr);
	}
	else {
		target = 0;
		obsz = FLASH_PAGE_SZ;
		buf_init(&obuf, obsz);
		printf("dnload file to dummy addr: 0x%08x ...\n", addr);
	}

	//init
	buf_init(&ibuf, 128);
	buf_init(&echo, 0);
	k = sys_malloc(sizeof(kermit_t));
	k -> ibuf = &ibuf;
	k -> obuf = &obuf;
	kermit_init(k, &ibuf, &obuf);
	length = 0; //file length
	while(1) {
		while(console_IsNotEmpty() && (buf_left(&ibuf) > 0)) {
			c = console_getch();
			buf_push(&ibuf, &c, 1);
			timeout = time_get(TIMEOUT);
		}

		ret = kermit_recv(k, &echo);

		sz = buf_size(&obuf);
		if(sz == obsz || ret == ERR_QUIT) {
			//copy data from obuf to target addr
			length += sz;
			if(target == 0) { //write to dummy
				buf_flush(&obuf);
				//mdelay(100);
			}
			else if(target == ROM) { //write to flash
				flash_Erase((void *)addr, 1);
				flash_Write((void *)addr, obuf.data, obsz);
				addr += sz;
				buf_flush(&obuf);
			}
		}

		while(buf_size(&echo) > 0) {
			//handle echo back
			buf_pop(&echo, &c, 1);
			console_putch(c);
		}
		
		if(ret == ERR_QUIT) {
			printf("recv file %s(len = %d) success\n", kermit_fname(k), length);
			break;
		}

		if(ret == ERR_ABORT) {
			printf("abort\n");
			break;
		}

		if(time_left(timeout) < 0) {
			printf("recv timeout, transfer abort\n");
			break;
		}
	}
	buf_free(&ibuf);
	buf_free(&obuf);
	buf_free(&echo);
	sys_free(k);
	return 0;
}

static int cmd_load_func(int argc, char *argv[])
{
	int addr;
	const char *usage = {
		"usage:\n"
		"load hex_addr	dnload binary file from current console\n"
	};

	if(argc >= 2) {
		sscanf(argv[1], "%x", &addr);
		return load_kermit(addr);
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_load = {"load", cmd_load_func, "download file from current console"};
DECLARE_SHELL_CMD(cmd_load)
