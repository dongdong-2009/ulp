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

#include "ulp/driver.h"
#include "lcd.h"
#include "lpt.h"

//regs
#define IDX 0
#define STA 0
#define DAT 1

static const lpt_bus_t *ssd_bus;

/*write indexed register*/
static int ssd_WriteRegister(int index, int dat)
{
	ssd_bus->write(IDX, index);;
	ssd_bus->write(DAT, dat);
	return 0;
}

/*read indexed register*/
static int ssd_ReadRegister(int index)
{
	ssd_bus->write(IDX,  index);;
	return ssd_bus->read(DAT);
}

static int ssd_SetWindow(int x0, int y0, int x1, int y1)
{
	int tmp;

	//set cursor
	ssd_WriteRegister(0x4e, x0); //x: 0~239
	ssd_WriteRegister(0x4f, y0); //y: 0~319

	#if 0
	//sort
	if(x1 < x0) {
		tmp = x1;
		x1 = x0;
		x0 = tmp;
	}

	if(y1 < y0) {
		tmp = y1;
		y1 = y0;
		y0 = tmp;
	}
	#endif

	//set window
	ssd_WriteRegister(0x44, (x1 << 8) | x0); //HEA|HSA
	ssd_WriteRegister(0x45, y0); //VSA
	ssd_WriteRegister(0x46, y1); //VEA

	//set index
	ssd_bus->write(IDX, 0X22);
	return 0;
}

static int ssd_wgram(const void *src, int n, int color)
{
	const unsigned short *p = src;
	if(src == NULL) {
		return ssd_bus ->writen(DAT, color, n);
	}

	return ssd_bus ->writeb(DAT, src, n);
}

static int ssd_rgram(void *dest, int n)
{
	unsigned short *p = dest;
	while (n > 0) {
		*p ++ = ssd_bus->read(DAT);
		n --;
	}
	return 0;
}

static int ssd_Initializtion(const struct lcd_cfg_s *cfg)
{
	int id;

	//lpt port init
	struct lpt_cfg_s lpt_cfg = LPT_CFG_DEF;
	lpt_cfg.mode = LPT_MODE_I80;
	lpt_cfg.t = 0;
	lpt_cfg.tp = 0;
	ssd_bus = cfg->bus;
	ssd_bus->init(&lpt_cfg);

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

	//lcd display direction setting
	ssd_WriteRegister(0x11,0x6030);

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
	return 0;
}

static const struct lcd_dev_s ssd = {
	.xres = 240,
	.yres = 320,

	.init = ssd_Initializtion,
	.setwindow = ssd_SetWindow,
	.rgram = ssd_rgram,
	.wgram = ssd_wgram,

	.writereg = ssd_WriteRegister,
	.readreg = ssd_ReadRegister,
};

static void ssd_reg(void)
{
	lcd_add(&ssd, "ssd1289", LCD_TYPE_PIXEL);
}
driver_init(ssd_reg);
