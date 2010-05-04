/*
 * 	miaofng@2010 initial version
*		- common remote control driver
*		- philips rc-5 protocol is supported
 */

#include "config.h"
#include "stm32f10x.h"
#include "key.h"
#include "key_rc.h"
#include "time.h"
#include <stddef.h>

/* TIM4 CH3, PB8
rc5:
	Tc = 1/Fc = 1/ (433Khz/12) = 1 / 36Khz
	T = 0.444mS = 16*Tc
	T0 = T1 = 4T
	Tframe = 113.7mS = 256T
	Twidth_max = 4x2T = 8T

timer setting:
	Fapb1 = TIM_clk = 72Mhz
	Tck_cnt = 1/(72Mhz/8) = 0.1111uS
*/

#define T_div	8
#define T		4000 //0.444mS/0.111uS
#define T_err	(1 << 10) //max error +/-512, about 10%
#define T_max	(16 * T) //note: Twidth_max = 8T
#define N_idl	20 //note: N_idl * T_max > Tframe(256T)

static int rckey_capture_init(void);
static int rckey_capture_getpulsewidth(void);

static key_t rckey;
static int rckey_bits_shift;
static char rckey_bits; //nr of bits received in shift register rckey_bits_shift
static char rckey_sm;
static short rckey_idl; //counter for nokey detection

#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
enum {
	SM_IDLE,
	RC5_SM_SYNC, /*sync pulse 11 has been received ..*/
	RC5_SM_1, /*last received bit is '1' */
	RC5_SM_00, /*last received bits is '00' */
	SM_ERROR,
	SM_READY,
};

void rc5_rx_bits(int pulsewidth)
{
	int width;

	//error handling
	width = pulsewidth;
	if(pulsewidth > 0) {
		pulsewidth += (T_err >> 1);
		pulsewidth &= ~(T_err - 1);
		width = pulsewidth / T;
	}

	//state machine
	switch(rckey_sm) {
		case SM_IDLE:
			rckey_idl = 0;
			rckey_bits = 0;
			rckey_bits_shift = 0;
			rckey_sm = RC5_SM_SYNC;
			break;

		case RC5_SM_SYNC: /*sync pulse has been received*/
			if(width == 4) {
				rckey_bits_shift = 0x03;
				rckey_bits += 2;
				rckey_sm = RC5_SM_1;
			}
			else if(width < 0) {
				rckey_idl ++;
				if(rckey_idl > N_idl) {
					rckey_idl = 0;
					rckey.value = 0;
					rckey.flag_nokey = 1;
				}
			}
			break;

		case RC5_SM_1:
			if(width == 4) {  /*2X2T width pulse, 1->1 has been received*/
				rckey_bits_shift <<= 1;
				rckey_bits_shift |= 0x01;
				rckey_bits += 1;
			}
			else if(width == 6) { /*3X2T width pulse, 1->00 has been received*/
				rckey_bits_shift <<= 2;
				rckey_bits_shift |= 0x00;
				rckey_bits += 2;
			}
			else if(width == 8) { /*4X2T width pulse, 1->01 has been received*/
				rckey_bits_shift <<= 2;
				rckey_bits_shift |= 0x01;
				rckey_bits += 2;
			}
			else if(width < 0)
				rckey_sm = SM_READY;
			else
				rckey_sm = SM_ERROR;
			break;

		case RC5_SM_00:
			if(width == 4) { /*2X2T width pulse, 0->0 has been received*/
				rckey_bits_shift <<= 1;
				rckey_bits_shift |= 0x00;
				rckey_bits += 1;
			}
			else if(width == 6) {/*3X2T width pulse, 1 has been received*/
				rckey_bits_shift <<= 1;
				rckey_bits_shift |= 0x01;
				rckey_bits += 1;
			}
			else if(width < 0)
				rckey_sm = SM_READY;
			else
				rckey_sm = SM_ERROR;
			break;

		case SM_ERROR:
			if(width < 0)
				rckey_sm = SM_IDLE;
			break;

		default:
			rckey_sm = SM_IDLE;
	}

	if(rckey_sm != SM_READY)
		return;

	rckey_sm = SM_IDLE;

	/*fix 'last bit 0 lost' situation*/
	if(rckey_bits == 13) {
		rckey_bits_shift <<= 1;
		rckey_bits_shift |= 0x00;
		rckey_bits += 1;
	}

	if(rckey_bits != 14)
		return;

	/*a complete 14bit rc5 frame is received*/
	rckey.value = 0;
	rckey.data = rckey_bits_shift & 0x3f; /*6 bits data code*/
	rckey_bits_shift >>= 6;
	rckey.data = rckey_bits_shift & 0x1f; /*5 bits custom code*/
	rckey_bits_shift >>= 5;
	rckey.flag_toggle = rckey_bits_shift & 0x01; /*1bit toggle flag*/
}
#else
#define rc5_rx_bits
#endif

int rckey_init(void)
{
	rckey_sm = SM_IDLE;
	rckey.value = 0;
	rckey.flag_nokey = 1;
	return rckey_capture_init();
}

key_t rckey_getkey(void)
{
	return rckey;
}

void rckey_isr(void)
{
	int pulsewidth;
	pulsewidth = rckey_capture_getpulsewidth();
	if(pulsewidth != 0) {
		rc5_rx_bits(pulsewidth);
	}
}

static int rckey_capture_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = T_max;
	TIM_TimeBaseStructure.TIM_Prescaler = T_div - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM4, ENABLE);

	TIM_ICInitStruct.TIM_Channel = TIM_Channel_3;
	TIM_ICInitStruct.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStruct.TIM_ICFilter = 0;
	TIM_ICInit(TIM4, &TIM_ICInitStruct);
	
	TIM_Cmd(TIM4, ENABLE);

	//irq init
	TIM_ClearFlag(TIM4, TIM_FLAG_CC3 | TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_CC3 | TIM_IT_Update, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	return 0;
}

static int rckey_capture_getpulsewidth(void)
{
	int width = 0;
	FlagStatus status;
	
	status = TIM_GetFlagStatus(TIM4, TIM_FLAG_CC3);
	if(status == SET) {
		width = TIM_GetCapture3(TIM4);
	}
	
	status = TIM_GetFlagStatus(TIM4, TIM_FLAG_Update);
	if(status == SET) {
		width = -1;
	}
	
	TIM_ClearFlag(TIM4, TIM_FLAG_CC3);
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	return width;
}
