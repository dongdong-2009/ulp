/*  encoder1_stm32.c
 *	dusk@2011 initial version
 */

#include "encoder.h"
#include "stm32f10x.h"

/*
 *@initialize timer1 ch-1 ch-2 to capture clock
 */
static void encoder_Init(const encoder_cfg_t *cfg)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	
	/* Configure PA.8 as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = cfg->max_value;	//defalut TIM_Period is 65535
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	//TIM_ARRPreloadConfig(TIM1, ENABLE);

	TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI1, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_SetCounter(TIM1,0);
	TIM_Cmd(TIM1, ENABLE);
}

static void encoder_SetCounter(int counter)
{
	TIM_Cmd(TIM1, DISABLE);
	//Sets the TIMx Autoreload Register value
	TIM_SetCounter(TIM1, counter);
	TIM_Cmd(TIM1, ENABLE);
}

static int encoder_GetCounter(void)
{
	return TIM_GetCounter(TIM1);
}

static void encoder_SetMaxValue(int value)
{
	//Sets the TIMx Autoreload Register value
	TIM_SetAutoreload(TIM1, value);
}

const encoder_t encoder1 = {
	.init = encoder_Init,
	.setCounter = encoder_SetCounter,
	.getCounter = encoder_GetCounter,
	.setMaxValue = encoder_SetMaxValue,
};

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "time.h"

//encoder state machine design with timer support
#define ENCODER_TIMEOUT			100		//300ms
#define ENCODER_DELTA_PARAM		500
enum encoder_state {
	Stable,Unstable,
};
static int init_counter, pre_counter, cur_counter;
static int delta;
static int counter_state;
static time_t init_timer, tim1, tim2;
static interval;

static int get_delta(const encoder_t *p)
{
	cur_counter = p->getCounter();
	if(cur_counter != pre_counter) {
		pre_counter = cur_counter;
		delta = 1;
	} else
		delta = 0;

	switch(counter_state) {
		case Stable:
			if(delta) {
				init_timer = time_get(0);
				tim1 = init_timer;
				tim2 = time_get(ENCODER_TIMEOUT);
				counter_state = Unstable;
			}
			break;

		case Unstable:
			if (delta) {
				tim1 = time_get(0);
				tim2 = time_get(ENCODER_TIMEOUT);
			} else {
				if (time_left(tim2) < 0) {
					counter_state = Stable;
					delta = cur_counter - init_counter;
					init_counter = cur_counter;						//init counter
					if (delta >= 10)
						delta = 100;
					else if (delta >= 5)
						delta = 10;
					//delta = (delta * ENCODER_DELTA_PARAM)/(tim1 - init_timer);
					interval = tim1 - init_timer;
					return delta;
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

static int cmd_encoder_func(int argc, char *argv[])
{
	encoder_cfg_t cfg;
	static const encoder_t *pencoder;
	int temp;
	int static rpm;

	const char *usage = { \
		"usage:\n " \
		"encoder init n val, init TIM encoder n with max value\n " \
		"encoder counter, get the number of pulses \n " \
		"encoder update, update the number of pulses \n " \
	};

	if(argc < 2 && argc != 0) {
		printf(usage);
		return 0;
	}

	if (argv[1][0] == 'i') {
		cfg.max_value = atoi(argv[3]);

		temp = atoi(argv[2]);
		if (temp == 1)
			pencoder = &encoder1;
		else if(temp == 2)
			pencoder = &encoder2;
		//else if(temp == 3)
			//pencoder = &encoder3;
		else if(temp == 4)
			pencoder = &encoder4;
		else {
			printf("no this encoder interface!\n");
			return 0;
		}

		pencoder->init(&cfg);
	}

	if (argv[1][0] == 'c') {
		printf("%d \n", pencoder->getCounter());
	}

	if (argc == 0 || argv[1][0] == 'u') {
		temp = get_delta(pencoder);
		rpm = rpm + temp;
		// if(temp)
			// printf("rpm: %d , delta: %d , interval: %d\n", rpm, temp, interval);
		// return 1;
		printf("%d \n", time_get(0));
		return 1;
	}

	return 0;
}

const cmd_t cmd_encoder = {"encoder", cmd_encoder_func, "encoder operation cmds"};
DECLARE_SHELL_CMD(cmd_encoder)

#endif