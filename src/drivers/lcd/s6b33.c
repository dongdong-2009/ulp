/*
* 	driver for samsung s6b33bc, which is a mid-display-size-compatible driver
*	for liquid crystal dot matrix gray-scale graphic systems.it's capable to driver
*	max 132RGBx162 dot matix with 65535 color out of 524288 color palettes.
*
*	miaofng@2010 initial version
*/
#include "config.h"
#include "ascii8x16.h"
#include "time.h"
#include "lcd.h"
#include "driver.h"
#include <string.h>

static unsigned short DeviceCode;
static unsigned short fgcolor;
static unsigned short bgcolor;

/* TSF8H0558FPC-A1-E (128x160) pin map:
1, LEDK -> 0v
2, LEDA1 ->3v3
3, LEDA2 ->3v3
4, GND -> 0v
5, GND -> 0v
6, VDD -> 3v3
7~8, MPU1~0, MPU i/f select,	MPU1-> 3v3, 4pin SPI mode
9, CS1B, low effective -> PB12/SPI2_NSS
10~25, DB15~DB0
18, DB7/SDI -> PB15/SPI2_MOSI
19, DB6/SCL -> PB13/SPI2_SCK
26, RDB
27, WRB
28, Register Select(high, DB0~15 = display data, low, instruction) -> PB14/SPI2_MISO
29, RSTB->low effective
30, MTPG
31, MTPD
32, CDIR->GND
*/
#include "stm32f10x.h"
static int lcm_hwInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef	SPI_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(spix, &SPI_InitStructure);
	/* Enable the SPI	*/
	SPI_Cmd(SPI2, ENABLE);
	SPI_SSOutputCmd(SPI2, ENABLE);
}

/*addr : 0->command, 1->display data*/
enum {
	CMD = Bit_RESET,
	DAT,
};

static int lcm_write(int type, int value)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_14, type);
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI2, value);
}

static int lcd_clear(void)
{
	int i, j;
	for(i = 0; i < 160; i ++) {
		for(j = 0; j < 128; j ++) {
			lcm_write(DAT, 0x03e0 >> 8);
			lcm_write(DAT, 0x03e0);
		}
	}
	return 0;
}

static int lcm_init(void)
{
	//*****STANDBY MODE OFF
	lcm_write(CMD, 0x2c);
	//*****OTP MODE ON/OFF********************
	// lcm_write(CMD, 0xeb); 
	// lcm_write(CMD, 0xea); 
	//*****OSCILLATION MODE SET***************
	lcm_write(CMD, 0x02);
	lcm_write(CMD, 0x01);
	mdelay(200);
	//***1'ST DC/DC SET TO 2X TIME BOOSTER***
	lcm_write(CMD, 0x20);
	lcm_write(CMD, 0x01);
	//mdelay(20);
	//*****BOOSTER1 ON************************
	lcm_write(CMD, 0x26);
	lcm_write(CMD, 0x01);
	mdelay(20);
	//******** BOOSTER1 ON AND AMP ON ********
	lcm_write(CMD, 0x26);
	lcm_write(CMD, 0x09);
	mdelay(20);
	//****************************************
	lcm_write(CMD, 0x26);
	lcm_write(CMD, 0x0b);
	mdelay(20);
	//****************************************
	lcm_write(CMD, 0x26);
	lcm_write(CMD, 0x0f);
	mdelay(20);
	//********DRIVER OUTPUT MODE SET**********
	lcm_write(CMD, 0x10);
	lcm_write(CMD, 0x23);
	//********MONITOR SIGNAL CONTROL**********
	lcm_write(CMD, 0x18);
	lcm_write(CMD, 0x00);
	//********BIAS SET************************
	lcm_write(CMD, 0x22);
	lcm_write(CMD, 0x12);
	//********DCDC CLOCK DIVISION SET**********
	lcm_write(CMD, 0x24);
	lcm_write(CMD, 0x00);
	//********TEMPERATURE COMPENSATION SET***
	lcm_write(CMD, 0x28);
	lcm_write(CMD, 0x00);
	//********CONTRAST CONTROL***************
	lcm_write(CMD, 0x2a);
	lcm_write(CMD, 0x5c);
	mdelay(100);
	lcm_write(CMD, 0x2a);
	lcm_write(CMD, 0x61);
	mdelay(100);
	lcm_write(CMD, 0x2a);
	lcm_write(CMD, 0x66);
	mdelay(100);
	lcm_write(CMD, 0x2a);
	lcm_write(CMD, 0x6b);
	mdelay(100);
	lcm_write(CMD, 0x2a);
	lcm_write(CMD, 0x70);
	mdelay(100);
	lcm_write(CMD, 0x2a);
	lcm_write(CMD, 0xa3);
	mdelay(100);
	lcm_write(CMD, 0x2b);
	lcm_write(CMD, 0x90);
	//********ADDRESSING MODE SET************
	lcm_write(CMD, 0x30);
	lcm_write(CMD, 0x17);
	//********N-LINE INVERSION SET************
	lcm_write(CMD, 0x34);
	lcm_write(CMD, 0x14);
	//********FRAME FREQUENCY CONTROL********
	lcm_write(CMD, 0x36);
	lcm_write(CMD, 0x00);
	//********ENTRY MODE SET*****************
	lcm_write(CMD, 0x40);
	lcm_write(CMD, 0x00);
	//********X ADDRESS SET FROM 0 TO 159******
	lcm_write(CMD, 0x42);
	lcm_write(CMD, 0x00);
	lcm_write(CMD, 0x9f);
	//********Y ADDRESS SET FROM 0 TO 127******
	lcm_write(CMD, 0x43);
	lcm_write(CMD, 0x00);
	lcm_write(CMD, 0x7f);
	//********N-LINE INVERSION SET************
	lcm_write(CMD, 0x34);
	lcm_write(CMD, 0x0b);
	//*********DISPLAY ON***********************
	lcm_write(CMD, 0x51);
	
	lcd_clear();
	return 0;
}

int ili9320_set_color(int fg, int bg)
{
	fgcolor = (unsigned short) fg;
	bgcolor = (unsigned short) bg;
}

static const lcd_t lcm = {
	.w = 20,//160/8
	.h = 8,//128/16
	.init = lcm_init,
	.puts = lcm_puts,
	.clear_all = lcm_clear,
	.clear_rect = NULL,
	.scroll = NULL,
	.set_color = lcm_set_color,
	.writereg = NULL,
	.readreg = NULL,
};

static void lcm_reg(void)
{
	fgcolor = (unsigned short) COLOR_FG_DEF;
	bgcolor = (unsigned short) COLOR_BG_DEF;
	lcm_hwInit();
	lcd_add(&lcm);
}
driver_init(lcm_reg);
