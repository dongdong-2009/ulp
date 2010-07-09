/*
 * 	miaofng@2010 initial version
 *		This file is used to debug kwp driver
 */

#include "config.h"
#include "kwp.h"
#include "shell/cmd.h"
#include "stdio.h"
#include "stdlib.h"

char htoc(char *buf)
{
	char h, l;

	h = buf[0];
	h = (h > 'a') ? (h - 'a' + 10) : h;
	h = (h > 'A') ? (h - 'A' + 10) : h;
	h = (h > '0') ? (h - '0') : h;

	l = buf[1];
	l = (l > 'a') ? (l - 'a' + 10) : l;
	l = (l > 'A') ? (l - 'A' + 10) : l;
	l = (l > '0') ? (l - '0') : l;

	return (h << 4) | l;
}

static char *buf;
static int cmd_kwp_func(int argc, char *argv[])
{
	int ret, n, i;
	const char usage[] = { \
		" usage:\n" \
		" kwp d0 d1 d2 ... dn\n" \
	};

	if(argc == 1) {
		printf(usage);
		return 0;
	}

	if(argc > 1) { //first time call
		ret = kwp_wake();
		if(!ret) {
			printf("kwp wakeup fail\n");
			return 0;
		}

		n = argc - 1;
		buf = kwp_malloc(n);
		for(i = 0; i < n; i ++) {
			buf[i] = htoc(argv[i + 1]);
		}

		return 1;
	}

	if(!kwp_isready())
		return 1;

	ret = kwp_transfer(buf, n);
	if(!ret) {
		printf("kwp transfer err\n");
		kwp_free(buf);
		return 0;
	}

	n = kwp_poll();
	if(n < 0) {
		printf("err occurs during kwp transfer\n");
		kwp_free(buf);
		return 0;
	} else if( n == 0) {
		printf(".");
		return 1;
	}

	//display the data received
	printf("%d bytes received: ", n);
	for(i = 0; i < n; i ++) {
		printf("%02X", buf[i]);
	}
	kwp_free(buf);
	return 0;
}

const cmd_t cmd_kwp = {"kwp", cmd_kwp_func, "common kwp driver debug"};
DECLARE_SHELL_CMD(cmd_kwp)
