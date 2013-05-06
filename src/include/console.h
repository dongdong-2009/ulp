/* console.h
 * 	miaofng@2009 initial version
 */
#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#include "config.h"

struct console_s {
	int (*init)(void *cfg); //NULL -> NEW STYLE DRIVER
	int (*putchar)(int data);
	int (*getchar)(void);
	int (*poll)(void); //return how many chars left in the rx buffer
	int fd;
};

const struct console_s* console_get(void);
int console_set(const struct console_s *);

void console_Init(void);
int console_putchar(char c);
int console_getchar(void);
int console_getch(void);
void console_flush(void);
int console_putch(char c);
int console_IsNotEmpty();

#ifdef CONFIG_SHELL_MULTI
int console_restore(void);
/*patch for new style device driver*/
struct console_s * console_register(int fd);
int console_unregister(struct console_s *cnsl);
#endif

#endif /*__CONSOLE_H_*/
