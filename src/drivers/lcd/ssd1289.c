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

#include "ascii16x32.h"
#include "time.h"
#include "lcd.h"
#include "driver.h"
#include <string.h>
#include "lpt.h"
#include "common/bitops.h"

//lcd width&height
#define _W 240
#define _H 320

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

void ssd_SetWindow(int x0, int y0, int x1, int y1)
{
	//virtual -> real
	lcd_transform(&x0, &y0, _W, _H);
	lcd_transform(&x1, &y1, _W, _H);

	//set cursor
	ssd_WriteRegister(0x4e, x0); //x: 0~239
	ssd_WriteRegister(0x4f, y0); //y: 0~319

	lcd_sort(&x0, &x1);
	lcd_sort(&y0, &y1);

	//set window
	ssd_WriteRegister(0x44, (x1 << 8) | x0); //HEA|HSA
	ssd_WriteRegister(0x45, y0); //VSA
	ssd_WriteRegister(0x46, y1); //VEA
}

/*clear the screen with the default bkcolor*/
static int ssd_Clear(void)
{
	int i;

#if CONFIG_LCD_ROT_090 == 1 || CONFIG_LCD_ROT_270 == 1
	ssd_SetWindow(0, 0, _H - 1, _W - 1);
#else
	ssd_SetWindow(0, 0, _W - 1, _H - 1);
#endif
	ssd_WriteIndex(0x22);
	for(i = 0; i < 320 * 240; i ++) {
		ssd_WriteData(bgcolor);
	}
	return 0;
}

int ssd_bitblt(const void *src, int x, int y, int w, int h)
{
	int i, j, v;
	ssd_SetWindow( x, y, x + w - 1, y + h - 1 );

	ssd_WriteIndex( 0x22 );
	for( j = 0; j < h; j ++ ) {
		for( i = 0; i < w; i ++ ) {
			v = bit_get( j * w + i, src );
			if( v )
				ssd_WriteData( fgcolor );
			else
				ssd_WriteData( bgcolor );
		}
	}

	return 0;
}

/*16x32*/
void ssd_PutChar(short x, short y, char c)
{
	const char *fontlib = ascii_16x32;

	fontlib += (c - '!' + 1) << 6;
	ssd_bitblt(fontlib, x, y, 16, 32);
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
	ssd_WriteRegister(0x01,0x233F); //driver output ctrl 320*240  0x6B3F
	ssd_WriteRegister(0x02,0x0600); //LCD Driving Waveform control
	ssd_WriteRegister(0x10,0x0000);

#if CONFIG_LCD_ROT_090 == 1
	ssd_WriteRegister(0x11,0x6018);
#elif CONFIG_LCD_ROT_180 == 1
	ssd_WriteRegister(0x11,0x6000);
#elif CONFIG_LCD_ROT_270 == 1
	ssd_WriteRegister(0x11,0x6028);
#else //CONFIG_LCD_ROT_000 == 1
	ssd_WriteRegister(0x11,0x6030);
#endif

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
#if CONFIG_LCD_ROT_090 == 1 || CONFIG_LCD_ROT_270 == 1
	.w = 20, //320/16
	.h = 7, //240/32
#else
	.w = 15, //240/16
	.h = 10, //320/32
#endif
	.init = ssd_Initializtion,
	.puts = ssd_WriteString,
	.clear_all = ssd_Clear,
	.clear_rect = NULL,
	.scroll = NULL,
	.set_color = ssd_set_color,
	.bitblt = ssd_bitblt,
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
