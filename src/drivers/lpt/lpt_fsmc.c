/*
*
*	miaofng@2013/1/14 initial version for smart board
*	ported from smart demo program
*/

#include "config.h"
#include "stm32f10x.h"
#include "lpt.h"
#include <string.h>

struct lpt_cfg_s lpt_cfg = LPT_CFG_DEF;

#define t0	((lpt_cfg.t - lpt_cfg.tp) >> 1)
#define t1	(lpt_cfg.tp)

#define ndelay(ns) do { \
	for(int i = ((ns) >> 3); i > 0 ; i -- ); \
} while(0)

#if CONFIG_LPT_A17 == 1
static inline void * __addr(unsigned addr)
{
	if(addr == 0) return (void *) 0x60000000;
	if(addr == 1) return (void *) 0x60040000;
	return (void *) addr;
}
#else /*CONFIG_LPT_A16*/
static inline void * __addr(unsigned addr)
{
	if(addr == 0) return (void *) 0x60000000;
	if(addr == 1) return (void *) 0x60020000;
	return (void *) addr;
}
#endif

static int lpt_init(const struct lpt_cfg_s *cfg)
{
	//default config
	if(cfg != NULL) {
		memcpy(&lpt_cfg, cfg, sizeof(struct lpt_cfg_s));
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | \
		GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | \
		GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*PD7 LCD_CS, FSMC_NE1 BANK1*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

#if CONFIG_LPT_A17 == 1
	/*PD12 LCD_RS FSMC_A17*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12;
#else
	/*PD11 LCD_RS FSMC_A16*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11;
#endif
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef FSMC_NORSRAMTimingInitStructure;
	/*FSMC read settings*/
	FSMC_NORSRAMTimingInitStructure.FSMC_AddressSetupTime = 10;
	FSMC_NORSRAMTimingInitStructure.FSMC_AddressHoldTime = 0;
	FSMC_NORSRAMTimingInitStructure.FSMC_DataSetupTime = 10;
	FSMC_NORSRAMTimingInitStructure.FSMC_BusTurnAroundDuration = 0x00;
	FSMC_NORSRAMTimingInitStructure.FSMC_CLKDivision = 0x00;
	FSMC_NORSRAMTimingInitStructure.FSMC_DataLatency = 0x00;
	FSMC_NORSRAMTimingInitStructure.FSMC_AccessMode = FSMC_AccessMode_A;

	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &FSMC_NORSRAMTimingInitStructure;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	/*fsmc write settings*/
	FSMC_NORSRAMTimingInitStructure.FSMC_AddressSetupTime = 10;
	FSMC_NORSRAMTimingInitStructure.FSMC_AddressHoldTime = 0;
	FSMC_NORSRAMTimingInitStructure.FSMC_DataSetupTime = 10;
	FSMC_NORSRAMTimingInitStructure.FSMC_BusTurnAroundDuration = 0x00;
	FSMC_NORSRAMTimingInitStructure.FSMC_CLKDivision = 0x00;
	FSMC_NORSRAMTimingInitStructure.FSMC_DataLatency = 0x00;
	FSMC_NORSRAMTimingInitStructure.FSMC_AccessMode = FSMC_AccessMode_A;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &FSMC_NORSRAMTimingInitStructure;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

	/* Enable FSMC Bank1_SRAM Bank */
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
	return 0;
}

static int lpt_write(int addr, int data)
{
	volatile unsigned short *p = __addr(addr);
	*p = (unsigned short) data;
	return 0;
}

static int lpt_writen(int addr, int data, int n)
{
	volatile unsigned short *p = __addr(addr);
	unsigned short v = (unsigned short) data;
	for(int i = 0; i < n; i ++) {
		*p = v;
	}
	return 0;
}

static int lpt_writeb(int addr, const void *buf, int n)
{
	volatile unsigned short *p = __addr(addr);
	unsigned short *data = (unsigned short *) buf;
	for(int i = 0; i < n; i ++) {
		*p = *data ++;
	}
	return 0;
}

static int lpt_read(int addr)
{
	volatile unsigned short *p = __addr(addr);
	return *p;
}

const lpt_bus_t lpt = {
	.init = lpt_init,
	.write = lpt_write,
	.read = lpt_read,
	.writeb = lpt_writeb,
	.writen = lpt_writen,
};
