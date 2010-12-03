/* console.h
 * 	miaofng@2009 initial version
 */
#ifndef __CONSOLE_H_
#define __CONSOLE_H_

void console_Init(void);
int console_putchar(char c);
int console_getchar(void);
int console_getch(void);
int console_putch(char c);
int console_IsNotEmpty();

#endif /*__CONSOLE_H_*/
