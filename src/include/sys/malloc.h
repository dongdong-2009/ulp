/**
* this file is provided in purpose to provide arch/os/compiler independent api
* for the whole software platform
*
* miaofng@2010 initial version
*
*/

#ifndef __SYS_MALLOC_H_
#define __SYS_MALLOC_H_

#include "config.h"
#include <stdlib.h>
#include <stdio.h>

//#define DEBUG_MALLOC

//memory management functions
#ifdef CONFIG_LIB_FREERTOS
#include "FreeRTOS.h"
#define sys_malloc pvPortMalloc
#define sys_free vPortFree
#else

#ifdef DEBUG_MALLOC
static inline void *debug_malloc(size_t size)
{
	void *p = malloc(size);
	printf("%08x + %d bytes\n", (int)p, size);
	return p;
}
static inline void debug_free(void *p)
{
	printf("%08x -\n", (int)p);
	free(p);
}
#define sys_malloc(X) (printf("%s:%d ", strrchr(__FILE__, '\\')+1, __LINE__), debug_malloc(X))
#define sys_free(X) (printf("%s:%d ", strrchr(__FILE__, '\\')+1, __LINE__), debug_free(X))
#else
#define sys_malloc malloc
#define sys_free free
#endif

#endif
#endif /*__SYS_MALLOC_H_*/
