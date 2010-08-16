#include "ascii8x16.h"
#include "time.h"
#include "lcd.h"
#include "driver.h"
#include <string.h>

#include "stm32f10x.h"
#include "ili9320.h"

static unsigned short DeviceCode;
static unsigned short fgcolor;
static unsigned short bgcolor;

static void Lcd_Configuration(void)
{
	/* pin map:
		PE0~15 <----> DB0~15
		PD15	 <----> nRD
		PD14	 <----> RS
		PD13	 <----> nWR
		PD12	 <----> nCS
		PD11	 <----> nReset
		PC0	<----> BK_LED
	*/
	
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB| \
		RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE, ENABLE);	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/*write index register*/
static void ili9320_WriteIndex(short idx)
{
	Clr_Rs;
	Set_nRd;
	GPIOE->ODR = idx;
	Clr_nWr;
	Set_nWr;
}

/*write data register*/
static void ili9320_WriteData(short data)
{
	Set_Rs;
	Set_nRd;
	GPIOE->ODR = data;
	Clr_nWr;
	Set_nWr;
}

/*read data register*/
static short ili9320_ReadData(void)
{
	unsigned short val = 0;
	Set_Rs;
	Set_nWr;
	Clr_nRd;
	GPIOE->CRH = 0x44444444;
	GPIOE->CRL = 0x44444444;
	val = GPIOE->IDR;
	val = GPIOE->IDR;
	GPIOE->CRH = 0x33333333;
	GPIOE->CRL = 0x33333333;
	Set_nRd;
	return val;
}

/*read indexed register*/
static int ili9320_ReadRegister(int index)
{ 
	Clr_Cs;
	ili9320_WriteIndex(index);		
	index = ili9320_ReadData();		
	Set_Cs;
	return index;
}

/*write indexed register*/
static int ili9320_WriteRegister(int index, int dat)
{
	Clr_Cs;
	ili9320_WriteIndex(index);			
	ili9320_WriteData(dat);		
	Set_Cs; 
	return 0;
}

void ili9320_SetCursor(short x, short y)
{
	if(DeviceCode == 0x8989) {
		ili9320_WriteRegister(0x004e, y); //row
		ili9320_WriteRegister(0x004f, 0x13f-x); //col
	}
	else if(DeviceCode == 0x9919) {
		ili9320_WriteRegister(0x004e, x); //row
		ili9320_WriteRegister(0x004f, y); //col	
	}
	else {
		ili9320_WriteRegister(0x0020, x); //row
		ili9320_WriteRegister(0x0021, y); //col
	}
}

void ili9320_SetWindows(short StartX, short StartY, short EndX, short EndY)
{
	ili9320_SetCursor(StartX, StartY);
	ili9320_WriteRegister(0x0050, StartX);
	ili9320_WriteRegister(0x0051, EndX);
	ili9320_WriteRegister(0x0052, StartY);
	ili9320_WriteRegister(0x0053, EndY);
}

static short ili9320_RGB2BGR(short c)
{
	short r, g, b, bgr = c;

	if(DeviceCode != 0x7783 && DeviceCode != 0x4531) {
		r = (c >> 0) & 0x1f;
		g = (c >> 5) & 0x3f;
		b = (c >> 11) & 0x1f;
	
		bgr = RGB565(b, g, r);
	}
	return bgr;
}

static short ili9320_BGR2RGB(short c)
{
	short r, g, b, rgb = c;
	if(DeviceCode != 0x7783 && DeviceCode != 0x4531) {
		b = (c >> 0) & 0x1f;
		g = (c >> 5) & 0x3f;
		r = (c >> 11) & 0x1f;
	
		rgb = RGB565(r, g, b);
	}
	return rgb;
}

static short ili9320_GetPoint(short x, short y)
{
	short temp;

	ili9320_SetCursor(x, y);
	Clr_Cs;
	ili9320_WriteIndex(0x0022);	
	temp = ili9320_ReadData(); //dummy
	temp = ili9320_ReadData(); 	
	
	Set_Cs;
	temp = ili9320_BGR2RGB(temp);
	return temp;
}

void ili9320_SetPoint(short x, short y, short point)
{
	ili9320_SetCursor(x, y);

	Clr_Cs;
	ili9320_WriteIndex(0x0022);
	ili9320_WriteData(point);
	Set_Cs;
}

/*clear the screen with the default bkcolor*/
static int ili9320_Clear(void)
{
	u32	i;

	ili9320_SetCursor(0x0000, 0x0000);	
	Clr_Cs; 
	ili9320_WriteIndex(0x0022);		
	for(i = 0; i < 76800; i ++){
		ili9320_WriteData(bgcolor);
	}
	Set_Cs;
	return 0;
}

int ili9320_DrawPicture(short StartX, short StartY, short EndX, short EndY, short *pic)
{
	short	i;

	ili9320_SetWindows(StartX, StartY, EndX, EndY);

	Clr_Cs;
	ili9320_WriteIndex(0x0022);
	for (i = 0; i < EndX * EndY; i ++) {
		ili9320_WriteData(*pic ++);
	}
	Set_Cs;
	
	return 0;
}

/*8x16 -> 16x32*/
void ili9320_PutChar(short x, short y, char c)
{
	int i, j;
	short v;
	const char *p = ascii_8x16 + ((c - '!' + 1) << 4);
	ili9320_SetWindows(x, y, x + 15, y + 31);
	
	Clr_Cs;
	ili9320_WriteIndex(0x0022);
	for (i = 0; i < 16; i ++) {
		c = *(p + i);
		for(j = 0; j < 8; j ++) {
			v = (c & 0x80) ? fgcolor : bgcolor;
			ili9320_WriteData(v);
			ili9320_WriteData(v);
			c <<= 1;
		}
		c = *(p + i);
		for(j = 0; j < 8; j ++) {
			v = (c & 0x80) ? fgcolor : bgcolor;
			ili9320_WriteData(v);
			ili9320_WriteData(v);
			c <<= 1;
		}
	}
	Set_Cs;
}

/************************************************************
write a string
x: 0~29
y: 0~19
*************************************************************/
int ili9320_WriteString(int x, int y, const char *s)
{
	while (*s) {
		ili9320_PutChar(x << 4 , y << 5, *s);
		s ++;
		x ++;
	}

	return 0;
}

int ili9320_set_color(int fg, int bg)
{
	fgcolor = ili9320_RGB2BGR((unsigned short) fg);
	bgcolor = ili9320_RGB2BGR((unsigned short) bg);
}

int ili9320_Initializtion(void)
{
	ili9320_WriteData(0xffff);
	Set_nWr;
	Set_Cs;
	Set_Rs;
	Set_nRd;

	ili9320_WriteRegister(0x0000,0x0001);
	mdelay(10); //wait at least 10ms to let the oscillator stable
	DeviceCode = ili9320_ReadRegister(0x0000);
	//	DeviceCode = 0x9320;
	if(DeviceCode==0x9325||DeviceCode==0x9328)
	{
		ili9320_WriteRegister(0x00e7,0x0010);			
		ili9320_WriteRegister(0x0000,0x0001);			//start internal osc
		ili9320_WriteRegister(0x0001,0x0100);		
		ili9320_WriteRegister(0x0002,0x0700); 				//power on sequence						
		ili9320_WriteRegister(0x0003,(1<<12)|(1<<5)|(1<<4) ); 	//65K 
		ili9320_WriteRegister(0x0004,0x0000);									
		ili9320_WriteRegister(0x0008,0x0207);				
		ili9320_WriteRegister(0x0009,0x0000);		
		ili9320_WriteRegister(0x000a,0x0000); 				//display setting		
		ili9320_WriteRegister(0x000c,0x0001);				//display setting			
		ili9320_WriteRegister(0x000d,0x0000); 				//0f3c			
		ili9320_WriteRegister(0x000f,0x0000);
		//Power On sequence //
		ili9320_WriteRegister(0x0010,0x0000);	 
		ili9320_WriteRegister(0x0011,0x0007);
		ili9320_WriteRegister(0x0012,0x0000);																
		ili9320_WriteRegister(0x0013,0x0000);				
		mdelay(1);
		ili9320_WriteRegister(0x0010,0x1590);	 
		ili9320_WriteRegister(0x0011,0x0227);
		mdelay(1);
		ili9320_WriteRegister(0x0012,0x009c);					
		mdelay(1);
		ili9320_WriteRegister(0x0013,0x1900);	 
		ili9320_WriteRegister(0x0029,0x0023);
		ili9320_WriteRegister(0x002b,0x000e);
		mdelay(1);
		ili9320_WriteRegister(0x0020,0x0000);																
		ili9320_WriteRegister(0x0021,0x0000);			
		///////////////////////////////////////////////////////			
		mdelay(1);
		ili9320_WriteRegister(0x0030,0x0007); 
		ili9320_WriteRegister(0x0031,0x0707);	 
		ili9320_WriteRegister(0x0032,0x0006);
		ili9320_WriteRegister(0x0035,0x0704);
		ili9320_WriteRegister(0x0036,0x1f04); 
		ili9320_WriteRegister(0x0037,0x0004);
		ili9320_WriteRegister(0x0038,0x0000);		
		ili9320_WriteRegister(0x0039,0x0706);		
		ili9320_WriteRegister(0x003c,0x0701);
		ili9320_WriteRegister(0x003d,0x000f);
		mdelay(1);
		ili9320_WriteRegister(0x0050,0x0000);		
		ili9320_WriteRegister(0x0051,0x00ef);	 
		ili9320_WriteRegister(0x0052,0x0000);		
		ili9320_WriteRegister(0x0053,0x013f);
		ili9320_WriteRegister(0x0060,0xa700);		
		ili9320_WriteRegister(0x0061,0x0001); 
		ili9320_WriteRegister(0x006a,0x0000);
		ili9320_WriteRegister(0x0080,0x0000);
		ili9320_WriteRegister(0x0081,0x0000);
		ili9320_WriteRegister(0x0082,0x0000);
		ili9320_WriteRegister(0x0083,0x0000);
		ili9320_WriteRegister(0x0084,0x0000);
		ili9320_WriteRegister(0x0085,0x0000);
			
		ili9320_WriteRegister(0x0090,0x0010);		
		ili9320_WriteRegister(0x0092,0x0000);	
		ili9320_WriteRegister(0x0093,0x0003);
		ili9320_WriteRegister(0x0095,0x0110);
		ili9320_WriteRegister(0x0097,0x0000);		
		ili9320_WriteRegister(0x0098,0x0000);	
		//display on sequence		
		ili9320_WriteRegister(0x0007,0x0133);
		
		ili9320_WriteRegister(0x0020,0x0000);																
		ili9320_WriteRegister(0x0021,0x0000);
	}
	else if(DeviceCode==0x9320)
	{
		ili9320_WriteRegister(0x00,0x0000);
		ili9320_WriteRegister(0x01,0x0100);	//Driver Output Contral.
		ili9320_WriteRegister(0x02,0x0700);	//LCD Driver Waveform Contral.
		ili9320_WriteRegister(0x03,0x1030);	//Entry Mode Set.
		//ili9320_WriteRegister(0x03, 0x1018);	//Entry Mode Set.
	
		ili9320_WriteRegister(0x04,0x0000);	//Scalling Contral.
		ili9320_WriteRegister(0x08,0x0202);	//Display Contral 2.(0x0207)
		ili9320_WriteRegister(0x09,0x0000);	//Display Contral 3.(0x0000)
		ili9320_WriteRegister(0x0a,0x0000);	//Frame Cycle Contal.(0x0000)
		ili9320_WriteRegister(0x0c,(1<<0));	//Extern Display Interface Contral 1.(0x0000)
		ili9320_WriteRegister(0x0d,0x0000);	//Frame Maker Position.
		ili9320_WriteRegister(0x0f,0x0000);	//Extern Display Interface Contral 2.
		mdelay(1);
		ili9320_WriteRegister(0x07,0x0101);	//Display Contral.
		mdelay(1);
		ili9320_WriteRegister(0x10,(1<<12)|(0<<8)|(1<<7)|(1<<6)|(0<<4));	//Power Control 1.(0x16b0)
		ili9320_WriteRegister(0x11,0x0007);								//Power Control 2.(0x0001)
		ili9320_WriteRegister(0x12,(1<<8)|(1<<4)|(0<<0));					//Power Control 3.(0x0138)
		ili9320_WriteRegister(0x13,0x0b00);								//Power Control 4.
		ili9320_WriteRegister(0x29,0x0000);								//Power Control 7.
	
		ili9320_WriteRegister(0x2b,(1<<14)|(1<<4));
		
		ili9320_WriteRegister(0x50,0);		//Set X Start.
		ili9320_WriteRegister(0x51,239);	//Set X End.
		ili9320_WriteRegister(0x52,0);		//Set Y Start.
		ili9320_WriteRegister(0x53,319);	//Set Y End.
	
		ili9320_WriteRegister(0x60,0x2700);	//Driver Output Control.
		ili9320_WriteRegister(0x61,0x0001);	//Driver Output Control.
		ili9320_WriteRegister(0x6a,0x0000);	//Vertical Srcoll Control.
	
		ili9320_WriteRegister(0x80,0x0000);	//Display Position? Partial Display 1.
		ili9320_WriteRegister(0x81,0x0000);	//RAM Address Start? Partial Display 1.
		ili9320_WriteRegister(0x82,0x0000);	//RAM Address End-Partial Display 1.
		ili9320_WriteRegister(0x83,0x0000);	//Displsy Position? Partial Display 2.
		ili9320_WriteRegister(0x84,0x0000);	//RAM Address Start? Partial Display 2.
		ili9320_WriteRegister(0x85,0x0000);	//RAM Address End? Partial Display 2.
	
		ili9320_WriteRegister(0x90,(0<<7)|(16<<0));	//Frame Cycle Contral.(0x0013)
		ili9320_WriteRegister(0x92,0x0000);	//Panel Interface Contral 2.(0x0000)
		ili9320_WriteRegister(0x93,0x0001);	//Panel Interface Contral 3.
		ili9320_WriteRegister(0x95,0x0110);	//Frame Cycle Contral.(0x0110)
		ili9320_WriteRegister(0x97,(0<<8));	//
		ili9320_WriteRegister(0x98,0x0000);	//Frame Cycle Contral.

		ili9320_WriteRegister(0x07,0x0173);	//(0x0173)
	}
	else if(DeviceCode==0x9919)
	{
		//------POWER ON &RESET DISPLAY OFF
		ili9320_WriteRegister(0x28,0x0006);		
		ili9320_WriteRegister(0x00,0x0001);
		ili9320_WriteRegister(0x10,0x0000);
		ili9320_WriteRegister(0x01,0x72ef);
		ili9320_WriteRegister(0x02,0x0600);
		ili9320_WriteRegister(0x03,0x6a38);
		ili9320_WriteRegister(0x11,0x6874);//70
		
		ili9320_WriteRegister(0x0f,0x0000); //	RAM WRITE DATA MASK
		ili9320_WriteRegister(0x0b,0x5308); //	RAM WRITE DATA MASK
		ili9320_WriteRegister(0x0c,0x0003);
		ili9320_WriteRegister(0x0d,0x000a);
		ili9320_WriteRegister(0x0e,0x2e00);	//0030
		ili9320_WriteRegister(0x1e,0x00be);
		ili9320_WriteRegister(0x25,0x8000);
		ili9320_WriteRegister(0x26,0x7800);
		ili9320_WriteRegister(0x27,0x0078);
		ili9320_WriteRegister(0x4e,0x0000);
		ili9320_WriteRegister(0x4f,0x0000);
		ili9320_WriteRegister(0x12,0x08d9);
		
		// -----------------Adjust the Gamma Curve----//
		ili9320_WriteRegister(0x30,0x0000);	 //0007
		ili9320_WriteRegister(0x31,0x0104);		//0203
		ili9320_WriteRegister(0x32,0x0100);		//0001
		ili9320_WriteRegister(0x33,0x0305);		//0007
		ili9320_WriteRegister(0x34,0x0505);		//0007
		ili9320_WriteRegister(0x35,0x0305);		//0407
		ili9320_WriteRegister(0x36,0x0707);		//0407
		ili9320_WriteRegister(0x37,0x0300);			//0607
		ili9320_WriteRegister(0x3a,0x1200);		//0106
		ili9320_WriteRegister(0x3b,0x0800);		
		ili9320_WriteRegister(0x07,0x0033);
	} 
	else if(DeviceCode==0x9331)
	{
		/*********POWER ON &RESET DISPLAY OFF*/
		/************* Start Initial Sequence **********/
		ili9320_WriteRegister(0x00E7, 0x1014);
		ili9320_WriteRegister(0x0001, 0x0100); // set SS and SM bit	 0x0100
		ili9320_WriteRegister(0x0002, 0x0200); // set 1 line inversion
		ili9320_WriteRegister(0x0003, 0x1030); // set GRAM write direction and BGR=1.		0x1030
		ili9320_WriteRegister(0x0008, 0x0202); // set the back porch and front porch
		ili9320_WriteRegister(0x0009, 0x0000); // set non-display area refresh cycle ISC[3:0]
		ili9320_WriteRegister(0x000A, 0x0000); // FMARK function
		ili9320_WriteRegister(0x000C, 0x0000); // RGB interface setting
		ili9320_WriteRegister(0x000D, 0x0000); // Frame marker Position
		ili9320_WriteRegister(0x000F, 0x0000); // RGB interface polarity
		//*************Power On sequence ****************//
		ili9320_WriteRegister(0x0010, 0x0000); // SAP, BT[3:0], AP, DSTB, SLP, STB
		ili9320_WriteRegister(0x0011, 0x0007); // DC1[2:0], DC0[2:0], VC[2:0]
		ili9320_WriteRegister(0x0012, 0x0000); // VREG1OUT voltage
		ili9320_WriteRegister(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude
		mdelay(10); // Dis-charge capacitor power voltage
		ili9320_WriteRegister(0x0010, 0x1690); // SAP, BT[3:0], AP, DSTB, SLP, STB
		ili9320_WriteRegister(0x0011, 0x0227); // DC1[2:0], DC0[2:0], VC[2:0]
		mdelay(1); // Delay 50ms
		ili9320_WriteRegister(0x0012, 0x000C); // Internal reference voltage= Vci;
		mdelay(1); // Delay 50ms
		ili9320_WriteRegister(0x0013, 0x0800); // Set VDV[4:0] for VCOM amplitude
		ili9320_WriteRegister(0x0029, 0x0011); // Set VCM[5:0] for VCOMH
		ili9320_WriteRegister(0x002B, 0x000B); // Set Frame Rate
		mdelay(1); // Delay 50ms
		ili9320_WriteRegister(0x0020, 0x0000); // GRAM horizontal Address
		ili9320_WriteRegister(0x0021, 0x0000); // GRAM Vertical Address
		// ----------- Adjust the Gamma Curve ----------//
		ili9320_WriteRegister(0x0030, 0x0000);
		ili9320_WriteRegister(0x0031, 0x0106);
		ili9320_WriteRegister(0x0032, 0x0000);
		ili9320_WriteRegister(0x0035, 0x0204);
		ili9320_WriteRegister(0x0036, 0x160A);
		ili9320_WriteRegister(0x0037, 0x0707);
		ili9320_WriteRegister(0x0038, 0x0106);
		ili9320_WriteRegister(0x0039, 0x0707);
		ili9320_WriteRegister(0x003C, 0x0402);
		ili9320_WriteRegister(0x003D, 0x0C0F);
		//------------------ Set GRAM area ---------------//
		ili9320_WriteRegister(0x0050, 0x0000); // Horizontal GRAM Start Address
		ili9320_WriteRegister(0x0051, 0x00EF); // Horizontal GRAM End Address
		ili9320_WriteRegister(0x0052, 0x0000); // Vertical GRAM Start Address
		ili9320_WriteRegister(0x0053, 0x013F); // Vertical GRAM Start Address
		ili9320_WriteRegister(0x0060, 0x2700); // Gate Scan Line
		ili9320_WriteRegister(0x0061, 0x0001); // NDL,VLE, REV
		ili9320_WriteRegister(0x006A, 0x0000); // set scrolling line
		//-------------- Partial Display Control ---------//
		ili9320_WriteRegister(0x0080, 0x0000);
		ili9320_WriteRegister(0x0081, 0x0000);
		ili9320_WriteRegister(0x0082, 0x0000);
		ili9320_WriteRegister(0x0083, 0x0000);
		ili9320_WriteRegister(0x0084, 0x0000);
		ili9320_WriteRegister(0x0085, 0x0000);
		//-------------- Panel Control -------------------//
		ili9320_WriteRegister(0x0090, 0x0010);
		ili9320_WriteRegister(0x0092, 0x0600);
		ili9320_WriteRegister(0x0007,0x0021);		
		mdelay(1);
		ili9320_WriteRegister(0x0007,0x0061);
		mdelay(1);
		ili9320_WriteRegister(0x0007,0x0133);	// 262K color and display ON
		mdelay(1);
	}	
	else if(DeviceCode==0x7783)
	{
		// Start Initial Sequence
		ili9320_WriteRegister(0x00FF,0x0001);
		ili9320_WriteRegister(0x00F3,0x0008);
		ili9320_WriteRegister(0x0001,0x0100);
		ili9320_WriteRegister(0x0002,0x0700);
		ili9320_WriteRegister(0x0003,0x1030);	//0x1030
		ili9320_WriteRegister(0x0008,0x0302);
		ili9320_WriteRegister(0x0008,0x0207);
		ili9320_WriteRegister(0x0009,0x0000);
		ili9320_WriteRegister(0x000A,0x0000);
		ili9320_WriteRegister(0x0010,0x0000);	//0x0790
		ili9320_WriteRegister(0x0011,0x0005);
		ili9320_WriteRegister(0x0012,0x0000);
		ili9320_WriteRegister(0x0013,0x0000);
		mdelay(1);
		ili9320_WriteRegister(0x0010,0x12B0);
		mdelay(1);
		ili9320_WriteRegister(0x0011,0x0007);
		mdelay(1);
		ili9320_WriteRegister(0x0012,0x008B);
		mdelay(1);
		ili9320_WriteRegister(0x0013,0x1700);
		mdelay(1);
		ili9320_WriteRegister(0x0029,0x0022);
		
		//################# void Gamma_Set(void) ####################//
		ili9320_WriteRegister(0x0030,0x0000);
		ili9320_WriteRegister(0x0031,0x0707);
		ili9320_WriteRegister(0x0032,0x0505);
		ili9320_WriteRegister(0x0035,0x0107);
		ili9320_WriteRegister(0x0036,0x0008);
		ili9320_WriteRegister(0x0037,0x0000);
		ili9320_WriteRegister(0x0038,0x0202);
		ili9320_WriteRegister(0x0039,0x0106);
		ili9320_WriteRegister(0x003C,0x0202);
		ili9320_WriteRegister(0x003D,0x0408);
		mdelay(1);

		ili9320_WriteRegister(0x0050,0x0000);		
		ili9320_WriteRegister(0x0051,0x00EF);		
		ili9320_WriteRegister(0x0052,0x0000);		
		ili9320_WriteRegister(0x0053,0x013F);		
		ili9320_WriteRegister(0x0060,0xA700);		
		ili9320_WriteRegister(0x0061,0x0001);
		ili9320_WriteRegister(0x0090,0x0033);				
		ili9320_WriteRegister(0x002B,0x000B);		
		ili9320_WriteRegister(0x0007,0x0133);
		mdelay(1);
	}	
	else if(DeviceCode==0x4531)
	{		
		// Setup display
		ili9320_WriteRegister(0x00,0x0001);
		ili9320_WriteRegister(0x10,0x0628);
		ili9320_WriteRegister(0x12,0x0006);
		ili9320_WriteRegister(0x13,0x0A32);
		ili9320_WriteRegister(0x11,0x0040);
		ili9320_WriteRegister(0x15,0x0050);
		ili9320_WriteRegister(0x12,0x0016);
		mdelay(1);
		ili9320_WriteRegister(0x10,0x5660);
		mdelay(1);
		ili9320_WriteRegister(0x13,0x2A4E);
		ili9320_WriteRegister(0x01,0x0100);
		ili9320_WriteRegister(0x02,0x0300);

		ili9320_WriteRegister(0x03,0x1030);
//		ili9320_WriteRegister(0x03,0x1038);
	
		ili9320_WriteRegister(0x08,0x0202);
		ili9320_WriteRegister(0x0A,0x0000);
		ili9320_WriteRegister(0x30,0x0000);
		ili9320_WriteRegister(0x31,0x0402);
		ili9320_WriteRegister(0x32,0x0106);
		ili9320_WriteRegister(0x33,0x0700);
		ili9320_WriteRegister(0x34,0x0104);
		ili9320_WriteRegister(0x35,0x0301);
		ili9320_WriteRegister(0x36,0x0707);
		ili9320_WriteRegister(0x37,0x0305);
		ili9320_WriteRegister(0x38,0x0208);
		ili9320_WriteRegister(0x39,0x0F0B);
		mdelay(1);
		ili9320_WriteRegister(0x41,0x0002);
		ili9320_WriteRegister(0x60,0x2700);
		ili9320_WriteRegister(0x61,0x0001);
		ili9320_WriteRegister(0x90,0x0119);
		ili9320_WriteRegister(0x92,0x010A);
		ili9320_WriteRegister(0x93,0x0004);
		ili9320_WriteRegister(0xA0,0x0100);
//		ili9320_WriteRegister(0x07,0x0001);
		mdelay(1);
//		ili9320_WriteRegister(0x07,0x0021); 
		mdelay(1);
//		ili9320_WriteRegister(0x07,0x0023);
		mdelay(1);
//		ili9320_WriteRegister(0x07,0x0033);
		mdelay(1);
		ili9320_WriteRegister(0x07,0x0133);
		mdelay(1);
		ili9320_WriteRegister(0xA0,0x0000);
		mdelay(1);
	} 					
	/*
	else if(DeviceCode==0x1505)
	{
// second release on 3/5	,luminance is acceptable,water wave appear during camera preview
		ili9320_WriteRegister(0x0007,0x0000);
		mdelay(5);
		ili9320_WriteRegister(0x0012,0x011C);//0x011A	 why need to set several times?
		ili9320_WriteRegister(0x00A4,0x0001);//NVM
		//
		ili9320_WriteRegister(0x0008,0x000F);
		ili9320_WriteRegister(0x000A,0x0008);
		ili9320_WriteRegister(0x000D,0x0008);
			
	//GAMMA CONTROL/
		ili9320_WriteRegister(0x0030,0x0707);
		ili9320_WriteRegister(0x0031,0x0007); //0x0707
		ili9320_WriteRegister(0x0032,0x0603); 
		ili9320_WriteRegister(0x0033,0x0700); 
		ili9320_WriteRegister(0x0034,0x0202); 
		ili9320_WriteRegister(0x0035,0x0002); //?0x0606
		ili9320_WriteRegister(0x0036,0x1F0F);
		ili9320_WriteRegister(0x0037,0x0707); //0x0f0f	0x0105
		ili9320_WriteRegister(0x0038,0x0000); 
		ili9320_WriteRegister(0x0039,0x0000); 
		ili9320_WriteRegister(0x003A,0x0707); 
		ili9320_WriteRegister(0x003B,0x0000); //0x0303
		ili9320_WriteRegister(0x003C,0x0007); //?0x0707
		ili9320_WriteRegister(0x003D,0x0000); //0x1313//0x1f08
		mdelay(5);
		ili9320_WriteRegister(0x0007,0x0001);
		ili9320_WriteRegister(0x0017,0x0001);	 //Power supply startup enable
		mdelay(5);
	
	//power control//
		ili9320_WriteRegister(0x0010,0x17A0); 
		ili9320_WriteRegister(0x0011,0x0217); //reference voltage VC[2:0]	 Vciout = 1.00*Vcivl
		ili9320_WriteRegister(0x0012,0x011E);//0x011c	//Vreg1out = Vcilvl*1.80	 is it the same as Vgama1out ?
		ili9320_WriteRegister(0x0013,0x0F00); //VDV[4:0]-->VCOM Amplitude VcomL = VcomH - Vcom Ampl
		ili9320_WriteRegister(0x002A,0x0000);	
		ili9320_WriteRegister(0x0029,0x000A); //0x0001F	Vcomh = VCM1[4:0]*Vreg1out		gate source voltage??
		ili9320_WriteRegister(0x0012,0x013E); // 0x013C	power supply on
			//Coordinates Control//
		ili9320_WriteRegister(0x0050,0x0000);//0x0e00
		ili9320_WriteRegister(0x0051,0x00EF); 
		ili9320_WriteRegister(0x0052,0x0000); 
		ili9320_WriteRegister(0x0053,0x013F); 
		//Pannel Image Control//
		ili9320_WriteRegister(0x0060,0x2700); 
		ili9320_WriteRegister(0x0061,0x0001); 
		ili9320_WriteRegister(0x006A,0x0000); 
		ili9320_WriteRegister(0x0080,0x0000); 
		//Partial Image Control//
		ili9320_WriteRegister(0x0081,0x0000); 
		ili9320_WriteRegister(0x0082,0x0000); 
		ili9320_WriteRegister(0x0083,0x0000); 
		ili9320_WriteRegister(0x0084,0x0000); 
		ili9320_WriteRegister(0x0085,0x0000); 
	//Panel Interface Control//
		ili9320_WriteRegister(0x0090,0x0013); //0x0010 frenqucy
		ili9320_WriteRegister(0x0092,0x0300); 
		ili9320_WriteRegister(0x0093,0x0005); 
		ili9320_WriteRegister(0x0095,0x0000); 
		ili9320_WriteRegister(0x0097,0x0000); 
		ili9320_WriteRegister(0x0098,0x0000); 
	
		ili9320_WriteRegister(0x0001,0x0100); 
		ili9320_WriteRegister(0x0002,0x0700); 
		ili9320_WriteRegister(0x0003,0x1030); 
		ili9320_WriteRegister(0x0004,0x0000); 
		ili9320_WriteRegister(0x000C,0x0000); 
		ili9320_WriteRegister(0x000F,0x0000); 
		ili9320_WriteRegister(0x0020,0x0000); 
		ili9320_WriteRegister(0x0021,0x0000); 
		ili9320_WriteRegister(0x0007,0x0021); 
		mdelay(20);
		ili9320_WriteRegister(0x0007,0x0061); 
		mdelay(20);
		ili9320_WriteRegister(0x0007,0x0173); 
		mdelay(20);
	}											
	else if(DeviceCode==0x8989)
	{
		ili9320_WriteRegister(0x0000,0x0001);		mdelay(50000);	//打开晶振
		ili9320_WriteRegister(0x0003,0xA8A4);		mdelay(50000);	 //0xA8A4
		ili9320_WriteRegister(0x000C,0x0000);		mdelay(50000);	 
		ili9320_WriteRegister(0x000D,0x080C);		mdelay(50000);	 
		ili9320_WriteRegister(0x000E,0x2B00);		mdelay(50000);	 
		ili9320_WriteRegister(0x001E,0x00B0);		mdelay(50000);	 
		ili9320_WriteRegister(0x0001,0x2B3F);		mdelay(50000);	 //驱动输出控制320*240	0x6B3F
		ili9320_WriteRegister(0x0002,0x0600);		mdelay(50000);
		ili9320_WriteRegister(0x0010,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0011,0x6070);		mdelay(50000);		//0x4030			//定义数据格式	16位色 		横屏 0x6058
		ili9320_WriteRegister(0x0005,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0006,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0016,0xEF1C);		mdelay(50000);
		ili9320_WriteRegister(0x0017,0x0003);		mdelay(50000);
		ili9320_WriteRegister(0x0007,0x0233);		mdelay(50000);		//0x0233			
		ili9320_WriteRegister(0x000B,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x000F,0x0000);		mdelay(50000);		//扫描开始地址
		ili9320_WriteRegister(0x0041,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0042,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0048,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0049,0x013F);		mdelay(50000);
		ili9320_WriteRegister(0x004A,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x004B,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0044,0xEF00);		mdelay(50000);
		ili9320_WriteRegister(0x0045,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0046,0x013F);		mdelay(50000);
		ili9320_WriteRegister(0x0030,0x0707);		mdelay(50000);
		ili9320_WriteRegister(0x0031,0x0204);		mdelay(50000);
		ili9320_WriteRegister(0x0032,0x0204);		mdelay(50000);
		ili9320_WriteRegister(0x0033,0x0502);		mdelay(50000);
		ili9320_WriteRegister(0x0034,0x0507);		mdelay(50000);
		ili9320_WriteRegister(0x0035,0x0204);		mdelay(50000);
		ili9320_WriteRegister(0x0036,0x0204);		mdelay(50000);
		ili9320_WriteRegister(0x0037,0x0502);		mdelay(50000);
		ili9320_WriteRegister(0x003A,0x0302);		mdelay(50000);
		ili9320_WriteRegister(0x003B,0x0302);		mdelay(50000);
		ili9320_WriteRegister(0x0023,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0024,0x0000);		mdelay(50000);
		ili9320_WriteRegister(0x0025,0x8000);		mdelay(50000);
		ili9320_WriteRegister(0x004f,0);
		ili9320_WriteRegister(0x004e,0);
	}	*/
	ili9320_Clear();
	return 0;
}

static const lcd_t lcd932x = {
	.w = 15,
	.h = 10,
	.init = ili9320_Initializtion,
	.puts = ili9320_WriteString,
	.clear_all = ili9320_Clear,
	.clear_rect = NULL,
	.scroll = NULL,
	.set_color = ili9320_set_color,
	.writereg = ili9320_WriteRegister,
	.readreg = ili9320_ReadRegister,
};

static void lcd932x_reg(void)
{
	fgcolor = (unsigned short) COLOR_FG_DEF;
	bgcolor = (unsigned short) COLOR_BG_DEF;
	Lcd_Configuration();
	lcd_add(&lcd932x);
}
driver_init(lcd932x_reg);
