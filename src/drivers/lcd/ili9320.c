/*
	ili9328, model nr: TM024HBZ13/S95417-AAA/KLD24A23
	SUCMAX ELECTRONIC CO.,LTD

	- A-SI TFT LCD MODULE
	- 2.4", 240X320, 262,000 COLOR
	- High speed RAM write function is available
	- Partial-screen display function is available

	miaofng@2010 initial version
*/

#include "time.h"
#include "lcd.h"
#include "driver.h"
#include "lpt.h"

//regs
#define IDX 0
#define STA 0
#define DAT 1

static lpt_bus_t *ili932x_bus;
static unsigned short ili932x_id;

/*write index register*/
static inline void ili932x_WriteIndex(int idx)
{
	ili932x_bus -> write(IDX, idx);
}

/*write data register*/
static inline void ili932x_WriteData(int data)
{
	ili932x_bus -> write(DAT, data);
}

/*read data register*/
static inline int ili932x_ReadData(void)
{
	return ili932x_bus -> read(DAT);
}

/*write graphic ram*/
static int ili932x_WriteGRAM(const void *src, int n)
{
	const unsigned short *p = src;
	while (n > 0) {
		ili932x_bus -> write(DAT, *p ++);
		n --;
	}
	return 0;
}

/*write graphic ram*/
static int ili932x_ReadGRAM(void *dest, int n)
{
	unsigned short *p = dest;
	while (n > 0) {
		*p ++ = ili932x_bus -> read(DAT);
		n --;
	}
	return 0;
}

/*read indexed register*/
static int ili932x_ReadRegister(int reg)
{
	ili932x_WriteIndex(reg);
	return ili932x_ReadData();
}

/*write indexed register*/
static int ili932x_WriteRegister(int reg, int dat)
{
	ili932x_WriteIndex(reg);
	ili932x_WriteData(dat);
	return 0;
}

void ili932x_SetCursor(int x, int y)
{
	ili932x_WriteRegister(0x0020, x); //row
	ili932x_WriteRegister(0x0021, y); //col
}

int ili932x_SetWindow(int x0, int y0, int x1, int y1)
{
	ili932x_SetCursor(x0, y0);
	ili932x_WriteRegister(0x0050, x0);
	ili932x_WriteRegister(0x0051, x1);
	ili932x_WriteRegister(0x0052, y0);
	ili932x_WriteRegister(0x0053, y1);
	ili932x_WriteIndex(0x0022);
	return 0;
}

int ili932x_Initializtion(const struct lcd_cfg_s *cfg)
{
	//lpt port init
	struct lpt_cfg_s lpt_cfg = LPT_CFG_DEF;
	lpt_cfg.mode = LPT_MODE_I80;
	lpt_cfg.t = 0;
	lpt_cfg.tp = 0;
	ili932x_bus = cfg -> bus;
	ili932x_bus -> init(&lpt_cfg);

	//start oscillator
	ili932x_WriteRegister(0x0000,0x0001);
	mdelay(10);

	//read dev code
	ili932x_id = ili932x_ReadRegister(0x0000);

	if(0) {
		int v, v1, v2;
		//register read/write test
		ili932x_WriteRegister(0x0020, 0x1122);
		ili932x_WriteRegister(0x0021, 0x3344);
		v1 = ili932x_ReadRegister(0x0020);
		v2 = ili932x_ReadRegister(0x0021);
		if(v1 != 0x1122 || v2 != 0x3344) {
			v = 1;
		}

		ili932x_SetWindow(0, 0, 240, 320);
		ili932x_WriteRegister(0x22, 0xaa00);
		ili932x_SetWindow(0, 0, 240, 320);
		v = ili932x_ReadRegister(0x22); //dummy read, transfer data from gram to RDR
		v = ili932x_ReadRegister(0x22); //transfer data from RDR to bus
		if(v != 0xaa00)
			return -1;
	}

	if(ili932x_id == 0x9325 || ili932x_id == 0x9328) {
		ili932x_WriteRegister(0x00e7, 0x0010);
		ili932x_WriteRegister(0x0000, 0x0001); //start internal osc
		ili932x_WriteRegister(0x0001, 0x0100);
		ili932x_WriteRegister(0x0002, 0x0700); //power on sequence
		ili932x_WriteRegister(0x0003, (0 << 12) | (1 << 5) | (1 << 4)); //65K
		ili932x_WriteRegister(0x0004, 0x0000);
		ili932x_WriteRegister(0x0008, 0x0207);
		ili932x_WriteRegister(0x0009, 0x0000);
		ili932x_WriteRegister(0x000a, 0x0000); //display setting
		ili932x_WriteRegister(0x000c, 0x0001); //display setting
		ili932x_WriteRegister(0x000d, 0x0000); //0f3c
		ili932x_WriteRegister(0x000f, 0x0000);
		//Power On sequence //
		ili932x_WriteRegister(0x0010, 0x0000);
		ili932x_WriteRegister(0x0011, 0x0007);
		ili932x_WriteRegister(0x0012, 0x0000);
		ili932x_WriteRegister(0x0013, 0x0000);
		mdelay(1);
		ili932x_WriteRegister(0x0010, 0x1590);
		ili932x_WriteRegister(0x0011, 0x0227);
		mdelay(1);
		ili932x_WriteRegister(0x0012, 0x009c);
		mdelay(1);
		ili932x_WriteRegister(0x0013, 0x1900);
		ili932x_WriteRegister(0x0029, 0x0023);
		ili932x_WriteRegister(0x002b, 0x000e);
		mdelay(1);
		ili932x_WriteRegister(0x0020, 0x0000);
		ili932x_WriteRegister(0x0021, 0x0000);

		mdelay(1);
		ili932x_WriteRegister(0x0030, 0x0007);
		ili932x_WriteRegister(0x0031, 0x0707);
		ili932x_WriteRegister(0x0032, 0x0006);
		ili932x_WriteRegister(0x0035, 0x0704);
		ili932x_WriteRegister(0x0036, 0x1f04);
		ili932x_WriteRegister(0x0037, 0x0004);
		ili932x_WriteRegister(0x0038, 0x0000);
		ili932x_WriteRegister(0x0039, 0x0706);
		ili932x_WriteRegister(0x003c, 0x0701);
		ili932x_WriteRegister(0x003d, 0x000f);
		mdelay(1);
		ili932x_WriteRegister(0x0050, 0x0000);
		ili932x_WriteRegister(0x0051, 0x00ef);
		ili932x_WriteRegister(0x0052, 0x0000);
		ili932x_WriteRegister(0x0053, 0x013f);
		ili932x_WriteRegister(0x0060, 0xa700);
		ili932x_WriteRegister(0x0061, 0x0001);
		ili932x_WriteRegister(0x006a, 0x0000);
		ili932x_WriteRegister(0x0080, 0x0000);
		ili932x_WriteRegister(0x0081, 0x0000);
		ili932x_WriteRegister(0x0082, 0x0000);
		ili932x_WriteRegister(0x0083, 0x0000);
		ili932x_WriteRegister(0x0084, 0x0000);
		ili932x_WriteRegister(0x0085, 0x0000);

		ili932x_WriteRegister(0x0090, 0x0010);
		ili932x_WriteRegister(0x0092, 0x0000);
		ili932x_WriteRegister(0x0093, 0x0003);
		ili932x_WriteRegister(0x0095, 0x0110);
		ili932x_WriteRegister(0x0097, 0x0000);
		ili932x_WriteRegister(0x0098, 0x0000);
		//display on sequence
		ili932x_WriteRegister(0x0007, 0x0133);

		ili932x_WriteRegister(0x0020, 0x0000);
		ili932x_WriteRegister(0x0021, 0x0000);
	}
	else if(ili932x_id==0x9320) {
		ili932x_WriteRegister(0x00, 0x0000);
		ili932x_WriteRegister(0x01, 0x0100); //Driver Output Contral.
		ili932x_WriteRegister(0x02, 0x0700); //LCD Driver Waveform Contral.
		ili932x_WriteRegister(0x03, 0x1030); //Entry Mode Set.
		//ili932x_WriteRegister(0x03, 0x1018); //Entry Mode Set.

		ili932x_WriteRegister(0x04, 0x0000); //Scalling Contral.
		ili932x_WriteRegister(0x08, 0x0202); //Display Contral 2.(0x0207)
		ili932x_WriteRegister(0x09, 0x0000); //Display Contral 3.(0x0000)
		ili932x_WriteRegister(0x0a, 0x0000); //Frame Cycle Contal.(0x0000)
		ili932x_WriteRegister(0x0c, (1 << 0)); //Extern Display Interface Contral 1.(0x0000)
		ili932x_WriteRegister(0x0d, 0x0000); //Frame Maker Position.
		ili932x_WriteRegister(0x0f, 0x0000); //Extern Display Interface Contral 2.
		mdelay(1);
		ili932x_WriteRegister(0x07, 0x0101); //Display Contral.
		mdelay(1);
		ili932x_WriteRegister(0x10, (1 << 12) | (0 << 8) | (1 << 7) | (1 << 6) | (0 << 4)); //Power Control 1.(0x16b0)
		ili932x_WriteRegister(0x11, 0x0007); //Power Control 2.(0x0001)
		ili932x_WriteRegister(0x12, (1 << 8) | (1 << 4) | (0 << 0)); //Power Control 3.(0x0138)
		ili932x_WriteRegister(0x13, 0x0b00); //Power Control 4.
		ili932x_WriteRegister(0x29, 0x0000); //Power Control 7.

		ili932x_WriteRegister(0x2b, (1 << 14) | (1 << 4));

		ili932x_WriteRegister(0x50, 0); //Set X Start.
		ili932x_WriteRegister(0x51, 239); //Set X End.
		ili932x_WriteRegister(0x52, 0); //Set Y Start.
		ili932x_WriteRegister(0x53, 319); //Set Y End.

		ili932x_WriteRegister(0x60, 0x2700); //Driver Output Control.
		ili932x_WriteRegister(0x61, 0x0001); //Driver Output Control.
		ili932x_WriteRegister(0x6a, 0x0000); //Vertical Srcoll Control.

		ili932x_WriteRegister(0x80, 0x0000); //Display Position? Partial Display 1.
		ili932x_WriteRegister(0x81, 0x0000); //RAM Address Start? Partial Display 1.
		ili932x_WriteRegister(0x82, 0x0000); //RAM Address End-Partial Display 1.
		ili932x_WriteRegister(0x83, 0x0000); //Displsy Position? Partial Display 2.
		ili932x_WriteRegister(0x84, 0x0000); //RAM Address Start? Partial Display 2.
		ili932x_WriteRegister(0x85, 0x0000); //RAM Address End? Partial Display 2.

		ili932x_WriteRegister(0x90, (0 << 7) | (16 << 0)); //Frame Cycle Contral.(0x0013)
		ili932x_WriteRegister(0x92, 0x0000); //Panel Interface Contral 2.(0x0000)
		ili932x_WriteRegister(0x93, 0x0001); //Panel Interface Contral 3.
		ili932x_WriteRegister(0x95, 0x0110); //Frame Cycle Contral.(0x0110)
		ili932x_WriteRegister(0x97, (0 << 8)); //
		ili932x_WriteRegister(0x98, 0x0000); //Frame Cycle Contral.

		ili932x_WriteRegister(0x07, 0x0173); //(0x0173)
	}
	else return -1;
	return 0;
}

static const struct lcd_dev_s ili932x = {
	.xres = 240,
	.yres = 320,

	.init = ili932x_Initializtion,
	.setwindow = ili932x_SetWindow,
	.rgram = ili932x_ReadGRAM,
	.wgram = ili932x_WriteGRAM,

	.writereg = ili932x_WriteRegister,
	.readreg = ili932x_ReadRegister,
};

static void ili932x_reg(void)
{
	lcd_add(&ili932x, "ili932x", LCD_TYPE_PIXEL);
}
driver_init(ili932x_reg);
