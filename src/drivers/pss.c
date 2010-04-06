/*
*	miaofng@2010 initial version
 *		vvt position and speed sensor signal generator
  *		provide vvt signal: ne58x cam1x cam4x_in cam4x_ext vss
*/

#include "stm32f10x.h"
#include "pss.h"
#include "ad9833.h"
#include "spi.h"

static int rpm_dds_write_reg(int reg, int val);
static int rpm_dds_read_reg(int reg);

static spi_t spi1 = {
	.addr = SPI1,
	.mode = SPI_MODE_POL_0| SPI_MODE_PHA_1| SPI_MODE_BW_16,
};

static ad9833_t rpm_dds = {
	.io = {
		.write_reg = rpm_dds_write_reg,
		.read_reg = rpm_dds_read_reg,
	},
	.option = AD9833_OPT_OUT_SQU,
};

/*pinmap:
	NE58X	PG2
	CAM1X	PG3
	CAM4X_IN	PG4
	CAM4X_EXT	PG5
*/
void pss_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;
	
	/*pulse output gpio pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/*chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*rpm irq init*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
	EXTI_InitStruct.EXTI_Line = EXTI_Line0;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/*chip init*/
	spi_Init(&spi1);
	ad9833_Init(&rpm_dds);
}

void pss_Update(void)
{
}

 /*1hz speed -> 2hz dsso -> 1rpm*/
void pss_SetSpeed(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&rpm_dds, fw);
}

void pss_SetVolt(pss_ch_t ch, short mv)
{
	uint16_t pin;
	BitAction val = Bit_RESET;
		
	if(mv > 0)
		val = Bit_SET;

	pin = 1 << (2 + ch);
	GPIO_WriteBit(GPIOG, pin, val);
}

int rpm_dds_write_reg(int reg, int val)
{
	int ret;
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET);
	ret = spi_Write(&spi1, reg, val);
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_SET);
	return ret;
}

int rpm_dds_read_reg(int reg)
{
	int ret;
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET);
	ret = spi_Read(&spi1, reg);
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_SET);
	return ret;
}

