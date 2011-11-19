/* config.h
 * 	miaofng@2009 initial version
 */
 
#ifndef __ZBAR_CONFIG_H_
#define __ZBAR_CONFIG_H_

#ifdef CONFIG_ZBAR_EAN
#define ENABLE_EAN 1
#endif

#ifdef CONFIG_ZBAR_CODE128
#define ENABLE_CODE128 1
#endif

#ifdef CONFIG_ZBAR_CODE39
#define ENABLE_CODE39 1
#endif

#ifdef CONFIG_ZBAR_I25
#define ENABLE_I25 1
#endif

#ifdef CONFIG_ZBAR_PDF417
#define ENABLE_PDF417 1
#endif

#ifdef CONFIG_ZBAR_QRCODE
#define ENABLE_QRCODE 1
#endif

#define HAVE_INTTYPES_H
#define ZBAR_VERSION_MAJOR	0
#define ZBAR_VERSION_MINOR	10

//sys/time.h
/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

#include "ulp_time.h"
static inline int gettimeofday(struct timeval *t, void *zone)
{
	t->tv_sec = 0;
	t->tv_usec = time_get(0) * 1000;
	return 0;
}

//for qrdectxt.c
#include <string.h>
#define iconv_t int
#define iconv_open(x, y) (-1)
#define iconv_close(x)
static inline size_t iconv(iconv_t cd,
	char **inbuf, int *inbytesleft,
		char **outbuf, int *outbytesleft)
{
	memcpy(*outbuf, *inbuf, *inbytesleft);
	return *inbytesleft;
}

#endif /*__ZBAR_CONFIG_H_*/
