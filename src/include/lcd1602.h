/* lcd1602.h
 * 	dusk@2010 initial version
 */
#ifndef __LCD1602_H_
#define __LCD1602_H_

#include "stm32f10x.h"

#define LCD1602_RW	GPIO_Pin_8
#define LCD1602_RS	GPIO_Pin_9
#define LCD1602_E	GPIO_Pin_11

#define LCD1602_PORT (GPIO_Pin_All&0x00ff)

#define LCD1602_COMMAND_OFFSCREEN 0X08
#define LCD1602_COMMAND_CLRSCREEN 0X01
#define LCD1602_COMMAND_SETMODE   0X38
#define LCD1602_COMMAND_SETPTMOVE 0X06
#define LCD1602_COMMAND_SETCUSOR  0X0C

int lcd1602_Init(void);
int lcd1602_WriteChar(int row,int column,int8_t ch);
int lcd1602_WriteString(int column, int row, const char *s);
int lcd1602_ClearScreen(void);

#endif /*__LCD1602_H_*/
