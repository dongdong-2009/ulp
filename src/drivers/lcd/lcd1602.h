/* lcd1602.h
 * 	dusk@2010 initial version
 *	miaofng@2010, introduce lpt driver as its bottom device
 */
#ifndef __LCD1602_H_
#define __LCD1602_H_

//REGS, A0 = RS A1 = R/W_
#define CMD 0X00 //00
#define STA 0X02 //10
#define DIN 0X01 // 01
#define DOU 0X03 // 11

#define LCD1602_COMMAND_OFFSCREEN 0X08
#define LCD1602_COMMAND_CLRSCREEN 0X01
#define LCD1602_COMMAND_SETMODE   0X38
#define LCD1602_COMMAND_SETPTMOVE 0X06
#define LCD1602_COMMAND_SETCUSOR  0X0C

typedef enum{
	Bit_Ok = 0,
	Bit_Busy
}lcd1602_status;

#endif /*__LCD1602_H_*/
