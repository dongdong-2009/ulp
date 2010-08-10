#ifndef __ILI9320_H_
#define __ILI9320_H_

#include "config.h"
#include "lcd.h"

#define COLOR_BG_DEF	RGB565(0xff, 0xff, 0xff)
#define COLOR_FG_DEF	RGB565(0x00, 0xff, 0x00)

/*硬件相关的宏定义*/
/********************************************************************************/
#define Set_Cs  GPIOC->BSRR  = 0x00000040;
#define Clr_Cs  GPIOC->BRR   = 0x00000040;

#define Set_Rs  GPIOD->BSRR  = 0x00002000;
#define Clr_Rs  GPIOD->BRR   = 0x00002000;

#define Set_nWr GPIOD->BSRR  = 0x00004000;
#define Clr_nWr GPIOD->BRR   = 0x00004000;

#define Set_nRd GPIOD->BSRR  = 0x00008000;
#define Clr_nRd GPIOD->BRR   = 0x00008000;
/********************************************************************************/

void ili9320_WriteIndex(u16 idx);
void ili9320_WriteData(u16 dat);
u16 ili9320_ReadData(void);
int ili9320_WriteRegister(int index, int dat);
int ili9320_ReadRegister(int index);

u16 ili9320_BGR2RGB(u16 c);
void ili9320_SetCursor(u16 x,u16 y);
#endif
