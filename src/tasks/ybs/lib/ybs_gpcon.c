/*
*	this files is used to provide basic ybs  hardware driver
 * 	junjun@2011 initial version
 */
#include "stm32f10x.h"
#include "ybs_gpcon.h"

static int gpcon_signal_save;

int gpcon_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//MISC
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	return 0;
}



int gpcon_signal(int sig, int ops)
{
	int mask = (1 << sig);
	BitAction ba;

	switch (ops) {
	case SENPR_ON:
		gpcon_signal_save |= mask;
		ba = Bit_SET;
		break;
	case SENPR_OFF:
		gpcon_signal_save &= ~mask;
		ba = Bit_RESET;
		break;
	case TOGGLE:
		gpcon_signal_save = gpcon_signal_save ^ mask;
		ba = (gpcon_signal_save & mask) ? Bit_SET : Bit_RESET;
		break;
	default:
		return -1;
	}

	switch(sig) {
	case SENSOR:
		GPIO_WriteBit(GPIOA, GPIO_Pin_11, ba);
		GPIO_WriteBit(GPIOA, GPIO_Pin_12, ba);
		break;
	default:
		return -1;
	}
	return 0;
}


