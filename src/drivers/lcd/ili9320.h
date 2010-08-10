#ifndef __ILI9320_H_
#define __ILI9320_H_

#include "config.h"
#include "lcd.h"

#define COLOR_BG_DEF	RGB565(0x10, 0x10, 0x10)
#define COLOR_FG_DEF	RGB565(0xff, 0xff, 0xff)

#define Set_Cs  GPIOC->BSRR  = 0x00000040;
#define Clr_Cs  GPIOC->BRR   = 0x00000040;

#define Set_Rs  GPIOD->BSRR  = 0x00002000;
#define Clr_Rs  GPIOD->BRR   = 0x00002000;

#define Set_nWr GPIOD->BSRR  = 0x00004000;
#define Clr_nWr GPIOD->BRR   = 0x00004000;

#define Set_nRd GPIOD->BSRR  = 0x00008000;
#define Clr_nRd GPIOD->BRR   = 0x00008000;

#endif
