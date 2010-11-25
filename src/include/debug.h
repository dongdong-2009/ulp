/* debug.h
 * 	miaofng@2009 initial version
 */
 
#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdio.h>
#include "sys/debug.h"

#define assert(x) do { \
	if(!(x)) { \
		printf("assert fault at %s, line %d of %s\n", \
			__FUNCTION__, __LINE__, __FILE__); \
		while(1); \
	} \
} while(0)

#endif /*__CONFIG_H_*/
