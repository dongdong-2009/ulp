/*
 * 	miaofng@2010 initial version
 *		This file is used to debug kwd driver
 */

#include "config.h"
#include "kwd.h"
#include "shell/cmd.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"

char htoc(char *buf)
{
	char h, l;

	h = buf[0];
	h = (h >= 'a') ? (h - 'a' + 10) : h;
	h = (h >= 'A') ? (h - 'A' + 10) : h;
	h = (h >= '0') ? (h - '0') : h;

	l = buf[1];
	l = (l >= 'a') ? (l - 'a' + 10) : l;
	l = (l >= 'A') ? (l - 'A' + 10) : l;
	l = (l >= '0') ? (l - '0') : l;

	return (h << 4) | l;
}

static int n;
static char *buf;
static time_t resend;
#define RESEND 5000 //5s
static int cmd_kwd_func(int argc, char *argv[])
{
	int i;
	const char usage[] = { \
		" usage:\n" \
		" kwd d0 d1 d2 ... dn\n" \
	};

	if(argc == 1) {
		printf(usage);
		return 0;
	}

	if(argc > 1) { //first time call
		kwd_init();

		n = argc - 1;
		buf = malloc(n);
		for(i = 0; i < n; i ++) {
			buf[i] = htoc(argv[i + 1]);
		}

		resend = time_get(RESEND);
		kwd_transfer(buf, n, buf, n);
		return 1;
	}

	i = kwd_poll(1);

	if(i == n) {
		if(time_left(resend) < 0) {
			printf("resend ...\n");
			kwd_transfer(buf, n, buf, n);
			resend = time_get(RESEND);
		}
	}

	if(i != 0) {
		printf("\r%03d bytes received: ", n - i);
		return 1;
	}

	//display the data received
	for(i = 0; i < n; i ++) {
		printf("%02X", buf[i]);
	}
	free(buf);
	return 0;
}

const cmd_t cmd_kwd = {"kwd", cmd_kwd_func, "common kwd driver debug"};
DECLARE_SHELL_CMD(cmd_kwd)
