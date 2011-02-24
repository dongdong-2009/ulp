/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_MESSAGE_H_
#define __NEST_MESSAGE_H_

#include <stdarg.h>

int nest_message_init(void);
int nest_message(const char *fmt, ...);

int cmd_nest_log_func(int argc, char *argv[]);
#endif
