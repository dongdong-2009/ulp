/*
	ili9161, model nr: WX1800F-M45
	WANXIN IMAGE INC

	- A-SI TFT LCD MODULE
	- 1.77", 128x160, 65,536 COLOR
	- 80-SYSTEM 8 BIT CPU I/F
	- High speed RAM write function is available
	- Partial-screen display function is available

	miaofng@2010 initial version
*/

#include "ascii8x16.h"
#include "ulp_time.h"
#include "lcd.h"
#include "driver.h"
#include <string.h>
#include "lpt.h"

//regs
#define IDX 0
#define STA 0
#define DAT 1

static lpt_bus_t *ili_bus;
static unsigned short fgcolor;
static unsigned short bgcolor;

/*write index register*/
static void ili_WriteIndex(int idx)
{
	int lsb, msb;
	lsb = idx & 0xff;
	msb = (idx >> 8) & 0xff;
	ili_bus -> write(IDX, msb);
	ili_bus -> write(IDX, lsb);
}

/*write data register*/
static void ili_WriteData(int data)
{
	int lsb, msb;
	lsb = data & 0xff;
	msb = (data >> 8) & 0xff;
	ili_bus -> write(DAT, msb);
	ili_bus -> write(DAT, lsb);
}

/*read data register*/
static int ili_ReadData(void)
{
	int lsb, msb;
	msb = ili_bus -> read(DAT);
	lsb = ili_bus -> read(DAT);
	return (msb << 8) | lsb;
}

/*write graphic ram*/
static int ili_WriteGRAM(const void *src, int n, int color)
{
	int lsb, msb;
	short *p = (short *)src;

	lsb = color & 0xff;
	msb = (color >> 8) & 0xff;
	while (n) {
		if(p != NULL) {
			lsb = *p & 0xff;
			msb = (*p >> 8) & 0xff;
			p ++;
		}
		ili_bus -> write(DAT, msb);
		ili_bus -> write(DAT, lsb);
		n --;
	}

	return 0;
}

/*write graphic ram*/
static int ili_ReadGRAM(void *dest, int n)
{
	int lsb, msb;
	short *p = dest;

	while (n) {
		msb = ili_bus -> read(DAT);
		lsb = ili_bus -> read(DAT);
		*p = (msb << 8) | lsb;
		p ++;
		n --;
	}

	return 0;
}

/*write indexed register*/
static int ili_WriteRegister(int index, int dat)
{
	ili_WriteIndex(index);
	ili_WriteData(dat);
	return 0;
}

/*read indexed register*/
static int ili_ReadRegister(int index)
{
	ili_WriteIndex(index);
	return ili_ReadData();
}

void ili_SetCursor(int x, int y)
{
	int v = (y << 8) | x;
	ili_WriteRegister(0x21, v);
}

int ili_SetWindow(int x0, int y0, int x1, int y1)
{
	int x, y;
	x = (x1 << 8) | x0;
	y = (y1 << 8) | y0;

	ili_SetCursor(x0, y0);
	ili_WriteRegister(0x16, x);
	ili_WriteRegister(0x17, y);

	ili_WriteIndex(0x22);
	return 0;
}

/*clear the screen with the default bkcolor*/
static int ili_Clear(void)
{
	int i;

	ili_SetCursor(0, 0);
	ili_WriteIndex(0x22);
	for(i = 0; i < 128 * 160; i ++) {
		ili_WriteData(bgcolor);
	}
	return 0;
}

int ili_DrawPicture(int x0, int y0, int x1, int y1, short *pic)
{
	ili_SetWindow(x0, y0, x1, y1);
	ili_WriteIndex(0x22);
	for (int i = 0; i < (x1 - x0) * (y1 - y0); i ++) {
		ili_WriteData(*pic ++);
	}

	return 0;
}

/*8x16 -> 16x32*/
void ili_PutChar(short x, short y, char c)
{
	int i, j;
	short v;
	const char *p = ascii_8x16 + ((c - '!' + 1) << 4);
	ili_SetWindow(x, y, x + 15, y + 31);

	ili_WriteIndex(0x22);
	for (i = 0; i < 16; i ++) {
		c = *(p + i);
		for(j = 0; j < 8; j ++) {
			v = (c & 0x80) ? fgcolor : bgcolor;
			ili_WriteData(v);
			ili_WriteData(v);
			c <<= 1;
		}
		c = *(p + i);
		for(j = 0; j < 8; j ++) {
			v = (c & 0x80) ? fgcolor : bgcolor;
			ili_WriteData(v);
			ili_WriteData(v);
			c <<= 1;
		}
	}
}

/************************************************************
write a string
x: 0~29
y: 0~19
*************************************************************/
int ili_WriteString(int x, int y, const char *s)
{
	while (*s) {
		ili_PutChar(x << 4 , y << 5, *s);
		s ++;
		x ++;
	}

	return 0;
}


int ili_Initializtion(const struct lcd_cfg_s *cfg)
{
	int id;

	//lpt port init
	struct lpt_cfg_s lpt_cfg = LPT_CFG_DEF;
	lpt_cfg.mode = LPT_MODE_I80;
	lpt_cfg.t = 0;
	lpt_cfg.tp = 0;
	ili_bus = cfg -> bus;
	ili_bus -> init(&lpt_cfg);

	//start oscillator
	ili_WriteRegister(0x00, 0x0001);
	mdelay(10);

	//read dev code and verify
	id = ili_ReadRegister(0x00);
	if(id != 0x9161)
		return -1;

#if 0
	//gram read/write self test
	ili_WriteRegister(0x21, 0x0000);
	ili_WriteRegister(0x22, 0xaa00);
	ili_WriteRegister(0x21, 0x0000);
	id = ili_ReadRegister(0x22); //dummy read, transfer data from gram to RDR
	id = ili_ReadRegister(0x22); //transfer data from RDR to bus
	if(id != 0xaa00)
		return -1;
#endif

	//init first time
	ili_WriteRegister(0x01, 0x0113); //driver output ctrl[SM = 0|  GS = 0 | SS = 1| NL = 10011]
	ili_WriteRegister(0x02, 0x0700); //lcd drive waveform control
	ili_WriteRegister(0x05, 0x0030); //entry mode setting

	ili_WriteRegister(0x25, 0x0000); //compare register 1
	ili_WriteRegister(0x26, 0x0000); //compare register 2
	ili_WriteRegister(0x08, 0x0202); //display control 2
	ili_WriteRegister(0x0A, 0x0000); //RGB input i/f ctrl 1
	ili_WriteRegister(0x0B, 0x0000); //frame cycle ctrl

	ili_WriteRegister(0x0C, 0x0000); //power ctrl 3
	ili_WriteRegister(0x0F, 0x0000); //gate scan ctrl
	ili_WriteRegister(0x21, 0x0000); //cursor set
	ili_WriteRegister(0x14, 0x9F00); //1st screen drive position
	ili_WriteRegister(0x4f, 0x0018); //oscillator ctrl register [ RADJ = 11000] 232Khz
	ili_WriteRegister(0x16, 0x7F00); //h-RAM window
	ili_WriteRegister(0x17, 0x9F00); //v-RAM window
	mdelay(50);
	ili_WriteRegister(0x03, 0x0000); //power ctrl 1
	mdelay(10);

	ili_WriteRegister(0x09, 0x0000); //power ctrl 2
	mdelay(10);
	ili_WriteRegister(0x0D, 0x0000); //power ctrl 4
	mdelay(200);
	ili_WriteRegister(0x0E, 0x0000); //power ctrl 5
	mdelay(10);

	//power settings
	ili_WriteRegister(0x03, 0x0110); //power ctrl 1[BT = 001 | DC = 000 | AP = 100]
	ili_WriteRegister(0x09, 0x0006); //power ctrl 2
	mdelay(10);
	ili_WriteRegister(0x0D, 0x0013); //power ctrl 4
	mdelay(50);
	ili_WriteRegister(0x0E, 0x3056); //power ctrl 5

	//init 2nd time
	ili_WriteRegister(0x30, 0x0000);// Gamma Control1, KP1KP0
	ili_WriteRegister(0x31, 0x0707);// Gamma Control2, KP3KP2
	ili_WriteRegister(0x32, 0x0707);// Gamma Control3, KP5KP4
	ili_WriteRegister(0x33, 0x0305);// Gamma Control4, RP1RP0
	ili_WriteRegister(0x34, 0x0007);// Gamma Control5, VRP1VRP0
	ili_WriteRegister(0x35, 0x0000);// Gamma Control6, KN1KN0
	ili_WriteRegister(0x36, 0x0007);// Gamma Control7, KN3KN2
	ili_WriteRegister(0x37, 0x0502);// Gamma Control8, KN5KN4
	ili_WriteRegister(0x3A, 0x1F00);// Gamma Control9, RN1RN0
	ili_WriteRegister(0x3B, 0x050E);

	//display on
	ili_WriteRegister(0x07, 0x0001); //disp ctrl 1
	mdelay(40);
	ili_WriteRegister(0x07, 0x0021);
	mdelay(40);
	ili_WriteRegister(0x07, 0x0023);
	mdelay(40);
	ili_WriteRegister(0x07, 0x0037);
	mdelay(40);

	ili_Clear();
	return 0;
}

static const struct lcd_dev_s ili = {
	.xres = 128,
	.yres = 160,
	.init = ili_Initializtion,
	.puts = ili_WriteString,

	.setwindow = ili_SetWindow,
	.rgram = ili_ReadGRAM,
	.wgram = ili_WriteGRAM,

	.writereg = ili_WriteRegister,
	.readreg = ili_ReadRegister,
};

static void ili_reg(void)
{
	fgcolor = (unsigned short) LCD_FGCOLOR_DEF;
	bgcolor = (unsigned short) LCD_BGCOLOR_DEF;
	lcd_add(&ili, "ili9161", LCD_TYPE_PIXEL);
}
driver_init(ili_reg);
