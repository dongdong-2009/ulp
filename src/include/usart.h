/*
 * File Name : md204l.h
 * Author    : dusk
 */

#ifndef __USART_H
#define __USART_H

void usart_Init(void);
int usart_putchar(char c);
int usart_getch(void);
int usart_getchar(void);
int usart_IsNotEmpty();

#endif /* __USART_H */
