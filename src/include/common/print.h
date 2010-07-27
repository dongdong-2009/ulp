/*
 * 	miaofng@2010 initial version
 *
 */
#ifndef __PRINT_H_
#define __PRINT_H_

#define PRINT_QUEUE_SIZE 8
#define PRINT_LINE_SIZE 64

//could be called by any thread
int print(const char *fmt, ...);

#endif /*__PRINT_H_*/
