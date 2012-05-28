/* debug.h
 * 	miaofng@2009 initial version
 */

#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdio.h>
#include "sys/debug.h"

#ifndef assert
#define assert(x) do { \
	if(!(x)) { \
		printf("assert fault at %s, line %d of %s\n", \
			__FUNCTION__, __LINE__, __FILE__); \
		while(1); \
	} \
} while(0)
#endif

static inline void dump(unsigned addr, const void *p, int bytes)
{
	unsigned v, i;
	const unsigned char *pbuf = p;
	for(i = 0; i < bytes; i ++) {
			v = (unsigned) (pbuf[i] & 0xff);
			if(i % 16 == 0)
				printf("0x%08x: %02x", addr + i, v);
			else if((i % 16 == 15) || (i == bytes - 1))
				printf(" %02x\n", v);
			else
				printf(" %02x", v);
	}
}

#endif /*__CONFIG_H_*/
