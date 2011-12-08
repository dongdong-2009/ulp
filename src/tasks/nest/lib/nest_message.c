/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "nvm.h"
#include "common/circbuf.h"
#include "sys/malloc.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ulp/debug.h"

#if CONFIG_NEST_LOG_SIZE > 0
static char nest_log_buf[CONFIG_NEST_LOG_SIZE] __nvm;
static circbuf_t nest_log;
#endif

int nest_message_init(void)
{
#if CONFIG_NEST_LOG_SIZE > 0
	nest_log.data = nest_log_buf;
	nest_log.totalsize = CONFIG_NEST_LOG_SIZE;
	buf_flush(&nest_log);
#endif
	return 0;
}

int nest_message(const char *fmt, ...)
{
	va_list ap;
	char *pstr;
	int n = 0;

	pstr = (char *) sys_malloc(256);
	assert(pstr != NULL);
	va_start(ap, fmt);
	n += vsnprintf(pstr + n, 256 - n, fmt, ap);
	va_end(ap);

	//output string to console or log buffer
	printf("%s", pstr);
#if CONFIG_NEST_LOG_SIZE > 0
	buf_push(&nest_log, pstr, n);
#endif
	sys_free(pstr);
	return 0;
}

int cmd_nest_log_func(int argc, char *argv[])
{
#if CONFIG_NEST_LOG_SIZE > 0
	int n;
	circbuf_t tmpbuf; //for view only purpose
	memcpy(&tmpbuf, &nest_log, sizeof(circbuf_t));
	char *pstr = sys_malloc(256);
	assert(pstr != NULL);
	do {
		n = buf_pop(&tmpbuf, pstr, 256);
		pstr[n] = 0;
		printf("%s", pstr);
	} while(n != 0);
	sys_free(pstr);
#else
	printf("not supported\n");
#endif
	return 0;
}
