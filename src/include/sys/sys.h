/**
* this file is provided in purpose to provide arch/os/compiler independent api
* for the whole software platform
* 
* miaofng@2010 initial version
*
*/

#ifndef __SYS_H_
#define __SYS_H_

#include "config.h"
#include <stdlib.h>
#include "FreeRTOS.h"

//memory management functions
#ifdef CONFIG_LIB_FREERTOS
#define sys_malloc pvPortMalloc
#define sys_free vPortFree
#else
#define sys_malloc malloc
#define sys_free free
#endif

#endif /*__SYS_H_*/
