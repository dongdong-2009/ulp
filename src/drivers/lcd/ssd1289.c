/*
	SSD1289, model nr: TFT8K0940FPC-A1-E/TC320

	- A-SI TFT LCD MODULE
	- 3.2", 240x320, 262K COLOR
	- 80-SYSTEM 16 BIT CPU I/F
	- On Chip 240x320x18bit GRAM

	huatong@2009 original author, ref:  http://www.ourdev.cn/bbs/bbs_content.jsp?
		bbs_sn=3796340&bbs_page_no=1&search_mode=4&search_text=sxndwg&bbs_id=9999
	miaofng@2010 revised for bldc platform
*/

#include "ascii8x16.h"
#include "time.h"
#include "lcd.h"
#include "driver.h"
#include <string.h>
#include "lpt.h"

//regs
#define IDX 0
#define STA 0
#define DAT 1

static lpt_bus_t *ssd_bus;
static unsigned short fgcolor;
static unsigned short bgcolor;

/*write index register*/
static void ssd_WriteIndex(int idx)
{
	ssd_bus -> write(IDX, idx);
}

/*write data register*/
static void ssd_WriteData(int data)
{
	ssd_bus -> write(DAT, data);
}

/*read data register*/
static int ssd_ReadData(void)
{
	return ssd_bus -> read(DAT);
}

/*write indexed register*/
static int ssd_WriteRegister(int index, int dat)
{
	ssd_WriteIndex(index);
	ssd_WriteData(dat);
	return 0;
}

/*read indexed register*/
static int ssd_ReadRegister(int index)
{
	ssd_WriteIndex(index);
	return ssd_ReadData();
}

void ssd_SetCursor(int x, int y)
{
#ifdef CONFIG_LCD_DIR_V
	ssd_WriteRegister(0x4e, x); //x: 0~239
	ssd_WriteRegister(0x4f, y); //y: 0~319
#else //CONFIG_LCD_DIR_H
	ssd_WriteRegister(0x4e, y); //x: 0~239
	ssd_WriteRegister(0x4f, x); //y: 0~319
#endif
}

void ssd_SetWindow(int x0, int y0, int x1, int y1)
{
	int x, y;

	x = (x1 << 8) | x0;
	y = (y1 << 8) | y0;

	ssd_SetCursor(x0, y0);
#ifdef CONFIG_LCD_DIR_V
	ssd_WriteRegister(0x44, x); //HEA|HSA
	ssd_WriteRegister(0x45, y0); //VSA
	ssd_WriteRegister(0x46, y1); //VEA
#else //CONFIG_LCD_DIR_H
	ssd_WriteRegister(0x44, y); //HEA|HSA
	ssd_WriteRegister(0x45, x0); //VSA
	ssd_WriteRegister(0x46, x1); //VEA
#endif
}

/*clear the screen with the default bkcolor*/
static int ssd_Clear(void)
{
	int i;

	ssd_SetCursor(0, 0);
	ssd_WriteIndex(0x22);
	for(i = 0; i < 320 * 240; i ++) {
		ssd_WriteData(bgcolor);
	}
	return 0;
}

int ssd_DrawPicture(int x0, int y0, int x1, int y1, short *pic)
{
	ssd_SetWindow(x0, y0, x1, y1);
	ssd_WriteIndex(0x22);
	for (int i = 0; i < (x1 - x0) * (y1 - y0); i ++) {
		ssd_WriteData(*pic ++);
	}

	return 0;
}

/*8x16 -> 16x32*/
void ssd_PutChar(short x, short y, char c)
{
	int i, j;
	short v;
	const char *p = ascii_8x16 + ((c - '!' + 1) << 4);
	ssd_SetWindow(x, y, x + 15, y + 31);

	ssd_WriteIndex(0x22);
	for (i = 0; i < 16; i ++) {
		c = *(p + i);
		for(j = 0; j < 8; j ++) {
			v = (c & 0x80) ? fgcolor : bgcolor;
			ssd_WriteData(v);
			ssd_WriteData(v);
			c <<= 1;
		}
		c = *(p + i);
		for(j = 0; j < 8; j ++) {
			v = (c & 0x80) ? fgcolor : bgcolor;
			ssd_WriteData(v);
			ssd_WriteData(v);
			c <<= 1;
		}
	}
}

/************************************************************
write a string
x: 0~29
y: 0~19
*************************************************************/
int ssd_WriteString(int x, int y, const char *s)
{
	while (*s) {
		ssd_PutChar(x << 4 , y << 5, *s);
		s ++;
		x ++;
	}

	return 0;
}

int ssd_set_color(int fg, int bg)
{
	fgcolor = (unsigned short) fg;
	bgcolor = (unsigned short) bg;
	return 0;
}

int ssd_Initializtion(void)
{
	int id;

	//read dev code and verify
	id = ssd_ReadRegister(0x00);
	if(id != 0x8989) //chip bug??? should be 0x1289
		return -1;

#if 0
	//gram read/write self test
	ssd_WriteRegister(0x4e, 0x0000); //xaddr
	ssd_WriteRegister(0x4f, 0x0000); //yaddr
	ssd_WriteRegister(0x22, 0x1234);
	ssd_WriteRegister(0x4e, 0x0000); //xaddr
	ssd_WriteRegister(0x4f, 0x0000); //yaddr
	id = ssd_ReadRegister(0x22); //dummy read, transfer data from gram to RDR
	id = ssd_ReadRegister(0x22); //transfer data from RDR to bus
	if(id != 0x1234)
		return -1;
#endif

	ssd_WriteRegister(0x00,0x0001);	//start oscillator
	ssd_WriteRegister(0x03,0xA8A4);
	ssd_WriteRegister(0x0C,0x0000);
	ssd_WriteRegister(0x0D,0x080C);
	ssd_WriteRegister(0x0E,0x2B00);
	ssd_WriteRegister(0x1E,0x00B0);
	ssd_WriteRegister(0x01,0x2B3F); //driver output ctrl 320*240  0x6B3F
	ssd_WriteRegister(0x02,0x0600); //LCD Driving Waveform control
	ssd_WriteRegister(0x10,0x0000);
	ssd_WriteRegister(0x11,0x6070);	//0x4030 //data format def: 16bit hori 0x6058
	ssd_WriteRegister(0x05,0x0000);
	ssd_WriteRegister(0x06,0x0000);
	ssd_WriteRegister(0x16,0xEF1C);
	ssd_WriteRegister(0x17,0x0003);
	ssd_WriteRegister(0x07,0x0233); //0x0233
	ssd_WriteRegister(0x0B,0x0000);
	ssd_WriteRegister(0x0F,0x0000);	//scan start addr
	ssd_WriteRegister(0x41,0x0000);
	ssd_WriteRegister(0x42,0x0000);
	ssd_WriteRegister(0x48,0x0000);
	ssd_WriteRegister(0x49,0x013F);
	ssd_WriteRegister(0x4A,0x0000);
	ssd_WriteRegister(0x4B,0x0000);
	ssd_WriteRegister(0x44,0xEF00);
	ssd_WriteRegister(0x45,0x0000);
	ssd_WriteRegister(0x46,0x013F);
	ssd_WriteRegister(0x30,0x0707);
	ssd_WriteRegister(0x31,0x0204);
	ssd_WriteRegister(0x32,0x0204);
	ssd_WriteRegister(0x33,0x0502);
	ssd_WriteRegister(0x34,0x0507);
	ssd_WriteRegister(0x35,0x0204);
	ssd_WriteRegister(0x36,0x0204);
	ssd_WriteRegister(0x37,0x0502);
	ssd_WriteRegister(0x3A,0x0302);
	ssd_WriteRegister(0x3B,0x0302);
	ssd_WriteRegister(0x23,0x0000);
	ssd_WriteRegister(0x24,0x0000);
	ssd_WriteRegister(0x25,0x8000);
	//ssd_WriteRegister(0x4f,0);        //yaddr
	//ssd_WriteRegister(0x4e,0);        //xaddr

	ssd_Clear();
	return 0;
}

static const lcd_t ssd = {
	.w = 20,
	.h = 7,
	.init = ssd_Initializtion,
	.puts = ssd_WriteString,
	.clear_all = ssd_Clear,
	.clear_rect = NULL,
	.scroll = NULL,
	.set_color = ssd_set_color,
	.writereg = ssd_WriteRegister,
	.readreg = ssd_ReadRegister,
};

static void ssd_reg(void)
{
	fgcolor = (unsigned short) COLOR_FG_DEF;
	bgcolor = (unsigned short) COLOR_BG_DEF;
	ssd_bus = &lpt;
	ssd_bus -> init();
	lcd_add(&ssd);
}
driver_init(ssd_reg);
