/*
 * 	miaofng@2010 initial version
 *
 */
#ifndef __PRINT_H_
#define __PRINT_H_

#include "config.h"

#define PRINT_QUEUE_SIZE 32
#define PRINT_LINE_SIZE 128

//could be called by any thread
#ifdef CONFIG_LIB_FREERTOS
int print(const char *fmt, ...);
#else
#include <stdio.h>
#define print printf
#endif

#endif /*__PRINT_H_*/
