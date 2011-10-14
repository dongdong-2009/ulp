/*
*	dusk@2010 initial version
*/

#include "stm32f10x.h"
#include "driver.h"
#include "lcd.h"
#include "lcd1602.h"
#include "ulp_time.h"
#include <string.h>

#define CONFIG_LCD_BUS_WIDTH	8

#if CONFIG_LCD_BUS_WIDTH == 8
#define LCD1602_CMD_READ_REG		(*(volatile unsigned char *)0X6C000002)
#define LCD1602_CMD_WRITE_REG		(*(volatile unsigned char *)0X6C000000)
#define LCD1602_DATA_READ_REG		(*(volatile unsigned char *)0X6C000003)
#define LCD1602_DATA_WRITE_REG		(*(volatile unsigned char *)0X6C000001)
#else
#define LCD1602_CMD_READ_REG		(*(volatile unsigned char *)(0X6C000002<<1))
#define LCD1602_CMD_WRITE_REG		(*(volatile unsigned char *)(0X6C000000<<1))
#define LCD1602_DATA_READ_REG		(*(volatile unsigned char *)(0X6C000003<<1))
#define LCD1602_DATA_WRITE_REG		(*(volatile unsigned char *)(0X6C000001<<1))
#endif

#define lcd1602_ReadCommand()			(LCD1602_CMD_READ_REG)
#define lcd1602_WriteCommand(command)	(LCD1602_CMD_WRITE_REG = (command))
#define lcd1602_ReadData() 				(LCD1602_DATA_READ_REG)
#define lcd1602_WriteData(data) 		(LCD1602_DATA_WRITE_REG = (data))

/*private members define*/
static void fsmc_Init(void);
static lcd1602_status lcd1602_ReadStaus(void);
static int lcd1602_wait(void);

int lcd1602_Init(void)
{
	fsmc_Init();

	mdelay(15);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	mdelay(5);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	mdelay(5);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);	
	
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);		//set mode
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_OFFSCREEN);	//turn off screen
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_CLRSCREEN);	//clear screeen
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_SETPTMOVE);	//set lcd1602 ram pointer
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_SETCUSOR);		//set screen display
	
	return 0;
}

int lcd1602_WriteChar(int row,int column,int8_t ch)
{
	uint8_t i=0;

	i = row ? (0xc0 + column):(0x80 + column);

	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(i);
	
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteData(ch);

	return 0;
}

int lcd1602_WriteString(int column, int row, const char *s)
{
	uint8_t i=0,size;
	size = (uint8_t)strlen(s);
	i = row ? (0xc0 + column):(0x80 + column);

	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(i);	
	for( i = 0 ; i < size ; i++){
		//check the busy bit
		lcd1602_wait();		
		lcd1602_WriteData(*s);
		s++;
	}

	return 0;	
}

int lcd1602_ClearScreen(void)
{
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_CLRSCREEN);	//clear screeen

	return 0;
}

static void fsmc_Init(void)
{
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  p;
	GPIO_InitTypeDef GPIO_InitStructure; 
  
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOG | RCC_APB2Periph_GPIOE |
                         RCC_APB2Periph_GPIOF, ENABLE);

	/* SRAM Data lines configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 |
                                GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |
                                GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | 
                                GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
  
	/* SRAM Address lines configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | 
									GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 | 
									GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | 
									GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13; 
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* NOE and NWE configuration */  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 |GPIO_Pin_5;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* NE4 configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; 
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/* FSMC Configuration */
	p.FSMC_AddressSetupTime = 2;
	p.FSMC_AddressHoldTime = 2;
	p.FSMC_DataSetupTime = 2;
	p.FSMC_BusTurnAroundDuration = 0;
	p.FSMC_CLKDivision = 0;
	p.FSMC_DataLatency = 0;
	p.FSMC_AccessMode = FSMC_AccessMode_A;

	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure); 

	/* Enable FSMC Bank1_SRAM Bank */
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
}

static lcd1602_status lcd1602_ReadStaus(void)
{
	uint16_t i=0;
	lcd1602_status status;
	
	i = lcd1602_ReadCommand();
	status = (i&0x0080) ? Bit_Busy : Bit_Ok;	

	return status;
}

static char lcd1602_fail;
static int lcd1602_wait(void)
{
	int ret = -1;
	time_t ms = time_get(10); //timeout = 10mS
	
	while(lcd1602_fail == 0) {
		if(!lcd1602_ReadStaus()) {
			ret = 0;
			break;
		}		
		if(time_left(ms) < 0) {
			lcd1602_fail = 1;
			break;
		}
	}
	
	return ret;
}

static const lcd_t lcd1602 = {
	.w = 16,
	.h = 2,
	.init = lcd1602_Init,
	.puts = lcd1602_WriteString,
	.clear_all = lcd1602_ClearScreen,
	.clear_rect = NULL,
	.scroll = NULL,
};

static void lcd1602_reg(void)
{
	lcd_add(&lcd1602);
}
driver_init(lcd1602_reg);
