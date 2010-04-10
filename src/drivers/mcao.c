/*
*	multi channel analog output module
*	miaofng@2010 initial version
*/

#include "stm32f10x.h"
#include "dac1x8s.h"
#include "spi.h"
#include "mcao.h"

static int dac0_write_reg(int reg, int val);

static const dac1x8s_t dac0 = {
	.io = {
		.write_reg = dac0_write_reg,
		.read_reg = 0,
	},
};

void mcao_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/*chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET); // cs_dac0 = 1

	/*chip init*/
	dac1x8s_Init(&dac0);
}

/*
	12bit dac resolution
	2500mv Vref
*/

#define MV_TO_CNT(mv) ((mv << 12) / 2500)

void mcao_SetVolt(int ch, int mv)
{
	int cnt = MV_TO_CNT(mv);
	dac1x8s_SetVolt(&dac0, ch, cnt);
}

int dac0_write_reg(int reg, int val)
{
	int ret;
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
	ret = spi_Write(1, val);
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET);
	return ret;
}
