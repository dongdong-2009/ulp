/*
*	dusk@2010 initial version
*/

#include "lcd1602.h"
#include "time.h"
#include <string.h>

typedef enum{
	Bit_Ok = 0,
	Bit_Busy
}lcd1602_status;

static lcd1602_status lcd1602_ReadStaus(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	uint16_t i=0;
	lcd1602_status status;
	
	/* Configure PC.0-PC.7 as Input floating */
	GPIO_InitStructure.GPIO_Pin = LCD1602_PORT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_ResetBits(GPIOC, LCD1602_RS);
	GPIO_SetBits(GPIOC, LCD1602_RW);
	GPIO_SetBits(GPIOC, LCD1602_E);
	
	i = GPIO_ReadInputData(GPIOC);
	status = (i&0x0080) ? Bit_Busy : Bit_Ok;

	GPIO_ResetBits(GPIOC, LCD1602_E);

	/* Configure PC.0-PC.7 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = LCD1602_PORT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	return status;
}

static void lcd1602_WriteCommand(uint8_t command)
{
	GPIO_InitTypeDef GPIO_InitStructure;	
	
	/* Configure PC.0-PC.7 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = LCD1602_PORT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC,LCD1602_PORT&command);
	GPIO_ResetBits(GPIOC,LCD1602_PORT&(~command));

	GPIO_ResetBits(GPIOC, LCD1602_RS);
	GPIO_ResetBits(GPIOC, LCD1602_RW);
	GPIO_SetBits(GPIOC, LCD1602_E);
	udelay(1);
	GPIO_ResetBits(GPIOC, LCD1602_E);
}

int lcd1602_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  
	/* Configure PC.8 PC.9 PC.11 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = LCD1602_RW | LCD1602_RS | LCD1602_E;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOC, LCD1602_E);

	mdelay(15);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	mdelay(5);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	mdelay(5);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);	
	
	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);		//set mode
	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(LCD1602_COMMAND_OFFSCREEN);	//turn off screen
	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(LCD1602_COMMAND_CLRSCREEN);	//clear screeen
	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(LCD1602_COMMAND_SETPTMOVE);	//set lcd1602 ram pointer
	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(LCD1602_COMMAND_SETCUSOR);		//set screen display
	
	return 0;
}

int lcd1602_WriteChar(int row,int column,int8_t ch)
{
	uint8_t i=0;

	i = row ? (0xc0 + column):(0x80 + column);

	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(i);
	
	//check the busy bit
	while(lcd1602_ReadStaus());
	GPIO_SetBits(GPIOC,LCD1602_PORT&ch);
	GPIO_ResetBits(GPIOC,LCD1602_PORT&(~ch));
	
	GPIO_SetBits(GPIOC, LCD1602_RS);
	GPIO_ResetBits(GPIOC, LCD1602_RW);
	GPIO_SetBits(GPIOC, LCD1602_E);
	udelay(1);
	GPIO_ResetBits(GPIOC, LCD1602_E); 

	return 0;
}

int lcd1602_WriteString(int row,int column,const char *s)
{
	uint8_t i=0,size;
	size = (uint8_t)strlen(s);
	
#if 0
	for( i = 0 ; i < size ; i++){
		lcd1602_WriteChar(row, column + i, *(s++));
	}
#endif

#if 1
	i = row ? (0xc0 + column):(0x80 + column);

	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(i);
	
	for( i = 0 ; i < size ; i++){
		//check the busy bit
		while(lcd1602_ReadStaus());
		GPIO_SetBits(GPIOC,LCD1602_PORT&(*s));
		GPIO_ResetBits(GPIOC,LCD1602_PORT&(~(*s)));
	
		GPIO_SetBits(GPIOC, LCD1602_RS);
		GPIO_ResetBits(GPIOC, LCD1602_RW);
		GPIO_SetBits(GPIOC, LCD1602_E);
		udelay(1);
		GPIO_ResetBits(GPIOC, LCD1602_E);
		s++;
	}
#endif

	return 0;	
}

int lcd1602_ClearScreen(void)
{
	//check the busy bit
	while(lcd1602_ReadStaus());
	lcd1602_WriteCommand(LCD1602_COMMAND_CLRSCREEN);	//clear screeen

	return 0;
}
