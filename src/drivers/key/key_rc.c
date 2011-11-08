/*
 * 	miaofng@2010 initial version
*		- common remote control driver
*		- philips rc-5 protocol is supported
 */

#include "config.h"
#include "stm32f10x.h"
#include "key.h"
#include "driver.h"
#include "ulp_time.h"
#include <stddef.h>
#include <stdio.h>

#define dbg(force) if(0|force)

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
#if CONFIG_RCKEY_PROTOCOL_PPM == 1
#define T_div	16
#else
#define T_div	8
#endif
#define T		4000 //0.444mS/0.111uS
#define T_err	(1 << 10) //max error +/-512, about 10%
#define T_max	(16 * T) //note: Twidth_max = 8T
#if CONFIG_RCKEY_PROTOCOL_PPM == 1
#define N_idl	10 //note: N_idl * T_max > Tframe, Tframe = 108ms
#else
#define N_idl	20 //note: N_idl * T_max > Tframe(256T)
#endif


static int rckey_capture_init(void);
static int rckey_capture_getpulsewidth(void);
unsigned char bit_rev(unsigned char ch);

static key_t rckey;
static int rckey_bits_shift;
static char rckey_bits; //nr of bits received in shift register rckey_bits_shift
static char rckey_sm;
static short rckey_idl; //counter for nokey detection

#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
enum {
	SM_IDLE,
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
	if(width > 0) {
		width += (T_err >> 1);
		width /= T;
		dbg(0) {
			printf("<%d>", width);
		}
	}

	//state machine
	switch(rckey_sm) {
		case SM_IDLE:
			dbg(0) {
				printf("\n");
			}
			rckey_idl = 0;
			rckey_bits = 0;
			rckey_bits_shift = 0;
			rckey_sm = RC5_SM_1;
			break;

		case RC5_SM_1:
			if(width >= 0 && rckey_bits == 0) { /*sync pulse received*/
				rckey_bits_shift = 1;
				rckey_bits = 1;
				break;
			}
			
			if(width == 4) {  /*2X2T width pulse, 1->1 has been received*/
				rckey_bits_shift <<= 1;
				rckey_bits_shift |= 0x01;
				rckey_bits += 1;
			}
			else if(width == 6) { /*3X2T width pulse, 1->00 has been received*/
				rckey_bits_shift <<= 2;
				rckey_bits_shift |= 0x00;
				rckey_bits += 2;
				rckey_sm = RC5_SM_00;
			}
			else if(width == 8) { /*4X2T width pulse, 1->01 has been received*/
				rckey_bits_shift <<= 2;
				rckey_bits_shift |= 0x01;
				rckey_bits += 2;
			}
			else if(width < 0) {
				if(rckey_bits > 1)
					rckey_sm = SM_READY;
				else {
					rckey_idl ++;
					if(rckey_idl > N_idl) {
						rckey_idl = 0;
						rckey.value = 0;
						rckey.flag_nokey = 1;
					}
				}
			}
			else {
				rckey_sm = SM_ERROR;
			}
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
				rckey_sm = RC5_SM_1;
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

	if (rckey_sm == SM_ERROR) {
		dbg(0) {
			printf("%d-%d ", pulsewidth, width);
		}
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
	dbg(0) {
		printf("%02X", rckey_bits_shift);
	}
	
	rckey.value = 0;
	rckey.rc5.data = rckey_bits_shift & 0x3f; /*6 bits data code*/
	rckey_bits_shift >>= 6;
	rckey.rc5.system = rckey_bits_shift & 0x1f; /*5 bits custom code*/
	rckey_bits_shift >>= 5;
	rckey.flag_toggle = rckey_bits_shift & 0x01; /*1bit toggle flag*/
	rckey_bits_shift >>= 1;
	rckey.rc5.edata = rckey_bits_shift & 0x01; /*1bit extend data bit*/
}
#elif CONFIG_RCKEY_PROTOCOL_PPM == 1
//for PPM,38KHZ
//1.12ms means 0, Fapb1 = 36MHz, Tck_cnt = (36/8 = 4.5)MHz, CC = 5040
//2.24ms means 1, Fapb1 = 36MHz, Tck_cnt = (36/8 = 4.5)MHz, CC = 10080
//9ms+4.5ms ,     Fapb1 = 36MHz, Tck_cnt = (36/8 = 4.5)MHz, CC = 60750
#define TIMER_PPM_DECODING_DELIMIT		8000
#define TIMER_PPM_PREAMBLE_DELIMIT		50000

//for remote control, bit flow is from "low to high"
#define RC_FEILE_CODE	0x00bf0000
enum {
	SM_IDLE,
	PPM_PREAMBLE,
	PPM_RECV,
	SM_ERROR,
	SM_READY,
};


const static unsigned char FEILE_CODE_TABLE[] = {
	KEY_PLAY,  KEY_DOWN,       KEY_UP,       KEY_NONE,
	KEY_ENTER, KEY_LEFT,       KEY_RIGHT,    KEY_NONE,
	KEY_0,     KEY_BACKWARD,   KEY_FORWARD,  KEY_NONE,
	KEY_1,     KEY_2,          KEY_3,        KEY_NONE,
	KEY_4,     KEY_5,          KEY_6,        KEY_NONE,
	KEY_7,     KEY_8,          KEY_9,        KEY_NONE
};

void ppm_rx_bits(int pulsewidth)
{
	int width;
	int i;
	width = pulsewidth;
	unsigned char *bit_reverse;

	//width = 3: preamble code
	//width = 1: means 1.12ms
	//width = 2: means 2.24ms
	if (pulsewidth > 0) {
		if (pulsewidth > TIMER_PPM_PREAMBLE_DELIMIT)
			width = 3;
		else
			width = pulsewidth > TIMER_PPM_DECODING_DELIMIT ? 2:1;
	}

	switch (rckey_sm) {
		case SM_IDLE:
			if (width > 0)
				rckey_sm = PPM_PREAMBLE;
			break;

		case PPM_PREAMBLE:
			if (width == 3)
				rckey_sm = PPM_RECV;
			else
				rckey_sm = SM_ERROR;
			break;

		case PPM_RECV:
			if (width == 2) {
				rckey_bits_shift <<= 1;
				rckey_bits_shift |= 0x01;
				rckey_bits ++;
			} else if (width == 1) {
				rckey_bits_shift <<= 1;
				rckey_bits ++;
			} else if (width == -1 && rckey_bits == 32) {
				bit_reverse = (unsigned char *)&rckey_bits_shift;
				for (i = 0; i < 4; i++)
					bit_reverse[i] = bit_rev(bit_reverse[i]);
				//feile rc validation check
				if ((rckey_bits_shift & 0xffff0000) != RC_FEILE_CODE)
					rckey_sm = SM_ERROR;
				else {
					//printf("%8x\n", rckey_bits_shift);
					//for rckey process
					rckey.value = 0;
					rckey.code = FEILE_CODE_TABLE[bit_reverse[1]];
					rckey.flag_nokey = 0;
					rckey_sm = SM_READY;
				}
			} else {
				rckey_sm = SM_ERROR;
			}
			break;

		case SM_ERROR:
			if (width < 0) {
				rckey_idl ++;
				if (rckey_idl > N_idl) {
					rckey.value = 0;
					rckey.flag_nokey = 1;
					rckey_idl = 0;
					rckey_bits_shift = 0;
					rckey_bits = 0;
					rckey_sm = SM_IDLE;
				}
			}
			break;

		case SM_READY:
			//for successive key
			if(width == 3) {
				rckey_idl = 0;
			}
			if (width < 0) {
				rckey_idl ++;
				if(rckey_idl > N_idl) {
					rckey.value = 0;
					rckey.flag_nokey = 1;
					rckey_idl = 0;
					rckey_bits_shift = 0;
					rckey_bits = 0;
					rckey_sm = SM_IDLE;
				}
			}
			break;

		default:
			rckey_sm = SM_IDLE;
			break;
	}

	return;
}
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
#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
		rc5_rx_bits(pulsewidth);
#elif CONFIG_RCKEY_PROTOCOL_PPM == 1
		ppm_rx_bits(pulsewidth);
#endif
	}
}

static int rckey_capture_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
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
	TIM_ICInitStruct.TIM_ICFilter = 0x0f;
	TIM_ICInit(TIM4, &TIM_ICInitStruct);
	
	TIM_Cmd(TIM4, ENABLE);
	TIM_UpdateRequestConfig(TIM4, TIM_UpdateSource_Global); //Setting UG won't lead to an UEV

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
	
	status = TIM_GetFlagStatus(TIM4, TIM_FLAG_Update);
	if(status == SET) {
		width = -1;
	}
	
	status = TIM_GetFlagStatus(TIM4, TIM_FLAG_CC3);
	if(status == SET) {
		width = TIM_GetCapture3(TIM4);
		//reset tim4 counter
		TIM_GenerateEvent(TIM4, TIM_EventSource_Update);
	}
	
	TIM_ClearFlag(TIM4, TIM_FLAG_CC3);
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	return width;
}

unsigned char bit_rev(unsigned char ch)
{
	ch = ((ch & 0xF0) >> 4) | ((ch & 0x0F) << 4);
	ch = ((ch & 0xCC) >> 2) | ((ch & 0x33) << 2);
	ch = ((ch & 0xAA) >> 1) | ((ch & 0x55) << 1);

	return ch;
}

static const keyboard_t key_rc = {
	.init = rckey_init,
	.getkey = rckey_getkey,
};

void rckey_reg(void)
{
	keyboard_Add(&key_rc, KEYBOARD_TYPE_REMOTE);
}
driver_init(rckey_reg);
