/*
*	this files is used to provide basic nest misc hardware driver
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "time.h"
#include "cncb.h"

static int cncb_signal_save;

/*gpio pinmap:
	DI	DUT_DET		PD2
	DO	SIG1		PC0
	DO	SIG2		PC1
	DO	SIG3		PC2
	DO	SIG4		PC3
	DO	SIG5		PC4
	DO	SIG6		PC5
	LSD	BAT		PC7
	LSD	IGN		PC8
	LSD	LSD		PC9
	LSD	LED_R/FLED_OUT	PC10
	LSD	LED_Y/RLED_OUT	PC11
	LSD	LED_G/PLED_OUT	PC12
*/

int cncb_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//DI
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//DO&LSD
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | \
		GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | \
		GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | \
		GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC, GPIO_InitStructure.GPIO_Pin);
	cncb_signal_save = 0;
	return 0;
}

//detect dut present or not, return 0 if dut is present
int cncb_detect(int debounce)
{
	int level;
	time_t deadline = time_get(debounce);
	while(time_left(deadline) > 0) {
		level = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2);
		if(level != Bit_SET)
			return 1;
	}
	return 0;
}

int cncb_signal(int sig, int ops)
{
	int mask = (1 << sig);
	BitAction ba;

	switch (ops) {
	case SIG_HI:
	case SIG_LO:
		ba = (BitAction) ops;
		break;
	case TOGGLE:
		cncb_signal_save = cncb_signal_save ^ mask;
		ba = (cncb_signal_save & mask) ? Bit_SET : Bit_RESET;
		break;
	default:
		return -1;
	}

	switch(sig) {
	case SIG1:
		GPIO_WriteBit(GPIOC, GPIO_Pin_0, ba);
		break;
	case SIG2:
		GPIO_WriteBit(GPIOC, GPIO_Pin_1, ba);
		break;
	case SIG3:
		GPIO_WriteBit(GPIOC, GPIO_Pin_2, ba);
		break;
	case SIG4:
		GPIO_WriteBit(GPIOC, GPIO_Pin_3, ba);
		break;
	case SIG5:
		GPIO_WriteBit(GPIOC, GPIO_Pin_4, ba);
		break;
	case SIG6:
		GPIO_WriteBit(GPIOC, GPIO_Pin_5, ba);
		break;
	case BAT:
		GPIO_WriteBit(GPIOC, GPIO_Pin_7, ba);
		break;
	case IGN:
		GPIO_WriteBit(GPIOC, GPIO_Pin_8, ba);
		break;
	case LSD:
		GPIO_WriteBit(GPIOC, GPIO_Pin_9, ba);
		break;
	case LED_F:
		GPIO_WriteBit(GPIOC, GPIO_Pin_10, ba);
		break;
	case LED_R:
		GPIO_WriteBit(GPIOC, GPIO_Pin_11, ba);
		break;
	case LED_P:
		GPIO_WriteBit(GPIOC, GPIO_Pin_12, ba);
		break;
	default:;
	}
	return 0;
}
