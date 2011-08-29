/* jhd204a.h
 * David@2011,8 initial version
 */
#ifndef __JHD204A_H_
#define __JHD204A_H_

//REGS, A0 = RS A1 = R/W
//RS = 0 : instruction reg, RS = 1 : data reg
//R/W = 0 : write, R/W = 1 : read
#define CMD 0X00 //00
#define STA 0X02 //10
#define DIN 0X01 //01
#define DOU 0X03 //11

#define JHD204A_COMMAND_OFFSCREEN 0X08
#define JHD204A_COMMAND_ONSCREEN  0X0C
#define JHD204A_COMMAND_CLRSCREEN 0X01
#define JHD204A_COMMAND_SETFUNC   0X38
#define JHD204A_COMMAND_SETPTMOVE  0X06

typedef enum{
	Bit_Ok = 0,
	Bit_Busy
}jhd204a_status;

#endif /*__JHD204A_H_*/
