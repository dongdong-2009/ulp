/*
*	king@2011 initial version
*/
#include "stm32f10x.h"
#include "sys/task.h"
#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i2c.h"

#define MA_INDICATOR_FIFOSIZE 4
#define MA_ID_ADDRESS 0
#define MA_COUNTER_ADDRESS 64

static enum {
	MA_INDICATOR_STM_NULL,
	MA_INDICATOR_STM_INIT_INDICATOR1,
	MA_INDICATOR_STM_INIT_INDICATOR2,
	MA_INDICATOR_STM_RUN,
	MA_INDICATOR_STM_OVER,
} ma_indicator_stm = MA_INDICATOR_STM_NULL;

static enum {
	LED_R,
	LED_G,
	LED_Y,
};

static unsigned short ma_indicator_fifo[MA_INDICATOR_FIFOSIZE];
static unsigned short ma_indicator_value[MA_INDICATOR_FIFOSIZE];
static time_t ma_indicator_timer;
static int n;
static unsigned char value[15];
static unsigned char ID[64];
static unsigned int counter;
static const i2c_cfg_t ma_i2c = {
	.speed = 100000,
	.option = 0,
};


static unsigned int Ma_Button_Check()
{
	unsigned int status = 0;
	status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)? 0xaaaaaaa0 : 0xaaaaaaa1;
	status |= GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9) ? 0xaaaaaaa0 : 0xaaaaaaa2;
	return status;
}

static int Ma_EEPROM_Operation(unsigned int address, unsigned char *buffer, int length, unsigned char operation)
{
	switch(operation) {
	case 0:
		if(i2cs.rbuf(0x50, address, 1, buffer, length))
			return -1;
		return 0;
	case 1:
		for(int i = 0; i < length; i++) {
			if(i2cs.wbuf(0x50, address, 1, buffer, 1))
				return -1;
			buffer++;
			address++;
			udelay(50000);
		}
		return 0;
	default:
		return -1;
	}
}

static int Ma_LED_Operation(int led, unsigned short operation)
{
	if(operation > 100)
		return -1;
	switch(led) {
	case LED_R:
		TIM_SetCompare3(TIM2, 0);
		TIM_SetCompare4(TIM2, 0);
		TIM_SetCompare2(TIM2, operation);
		return 0;
	case LED_Y:
		TIM_SetCompare2(TIM2, 0);
		TIM_SetCompare4(TIM2, 0);
		TIM_SetCompare3(TIM2, operation);
		return 0;
	case LED_G:
		TIM_SetCompare2(TIM2, 0);
		TIM_SetCompare3(TIM2, 0);
		TIM_SetCompare4(TIM2, operation);
		return 0;
	default:
		return -1;
	}
}

static int Ma_Device_Operation(int device, int operation)
{
	switch(device) {
	case 1:
		if(operation)
			GPIO_SetBits(GPIOC, GPIO_Pin_6);
		else
			GPIO_ResetBits(GPIOC, GPIO_Pin_6);
		return 0;
	case 2:
		if(operation)
			GPIO_SetBits(GPIOC, GPIO_Pin_7);
		else
			GPIO_ResetBits(GPIOC, GPIO_Pin_7);
		return 0;
	default:
		return -1;
	}
}

static int Ma_Data_Handle(unsigned short *data, unsigned char *v)
{
	int i, j, p;
	unsigned char m[12];
	if(data[0] != 0xffff) {
		printf("error:	error data\n");
		return -1;
	}
	for(i = 1;i < 4; i++) {
		for(j = 0; j < 4; j++) {
			m[(i - 1) * 4 + j] = (data[i] >> (j * 4)) & 0x000f;
		}
	}
	switch(m[0]) {
	case 0:
		v[0] = '+';
		break;
	case 8:
		v[0] = '-';
		break;
	default:
		printf("error:	error data\n");
		return -1;
	}
	for(i = 1, j = 0; i <= (6 - m[7]); i++) {
		if(m[i] && !j)
			v[++j] = '0' + m[i];
		else if(j)
			v[++j] = '0' + m[i];
	}
	if(!j) {
		v[1] = '0';
		v[2] = '.';
		p = 3;
	}
	else {
		v[++j] = '.';
		p = j + 1;
	}
	for(i = 7 - m[7]; i < 7; i++)
		v[p++] = '0' + m[i];
	v[p++] = ' ';
	switch(m[8]) {
	case 0:
		v[p++] = 'm';
		v[p++] = 'm';
		v[p] = 0;
		break;
	case 1:
		v[p++] = 'i';
		v[p++] = 'n';
		v[p++] = 'c';
		v[p++] = 'h';
		v[p] = 0;
		break;
	default:
		printf("error:	error data\n");
		return -1;
	}
	return 0;
}

static void Ma_EEPROM_Init()
{
	i2cs.init(&ma_i2c);
}

static void Ma_LED_Init()
{
	/*	PA1:LED_R	PA2:LED_Y	PA3:LED_G	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_Cmd(TIM2, DISABLE);
	TIM_DeInit(TIM2);

	TIM_TimeBaseStructure.TIM_Period = 100;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM2, ENABLE);

	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);

	TIM_Cmd(TIM2, ENABLE);
}

static void Ma_Device_Init()
{
	/*	PC6:device1	PC7:device2	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_ResetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_7);
}

static void Ma_Button_Init()
{
	/*	PC8:START	PC9:STOP	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
}

static void Ma_Indicator_Init()
{
	/*	RD1	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*	REQ1	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*	REQ2	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	GPIO_SetBits(GPIOB, GPIO_Pin_11);
	GPIO_SetBits(GPIOC, GPIO_Pin_4);

	/*	SCK	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*	DATA	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*	NSS	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*	SPI Initial	*/
	SPI_Cmd(SPI1, DISABLE);
	SPI_I2S_DeInit(SPI1);
	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_RxOnly;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
	SPI_InitStructure.SPI_CRCPolynomial = 0;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_CalculateCRC(SPI1, DISABLE);

	/*	DMA Initial	*/
	DMA_InitTypeDef  DMA_InitStructure;
	DMA_Cmd(DMA1_Channel2, DISABLE);
	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&SPI1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned)ma_indicator_fifo;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = MA_INDICATOR_FIFOSIZE;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
	DMA_Cmd(DMA1_Channel2, ENABLE);
}

static void Ma_Indicator_Update()
{
	switch(ma_indicator_stm) {
	case MA_INDICATOR_STM_NULL:
		break;
	case MA_INDICATOR_STM_INIT_INDICATOR1:
		ma_indicator_stm = MA_INDICATOR_STM_RUN;
		SPI_Cmd(SPI1, ENABLE);
		DMA_Cmd(DMA1_Channel2, ENABLE);
		GPIO_ResetBits(GPIOB, GPIO_Pin_11);
		ma_indicator_timer = time_get(500);
		break;
	case MA_INDICATOR_STM_INIT_INDICATOR2:
		ma_indicator_stm = MA_INDICATOR_STM_RUN;
		SPI_Cmd(SPI1, ENABLE);
		DMA_Cmd(DMA1_Channel2, ENABLE);
		GPIO_ResetBits(GPIOC, GPIO_Pin_4);
		ma_indicator_timer = time_get(500);
		break;
	case MA_INDICATOR_STM_RUN:
		n = DMA_GetCurrDataCounter(DMA1_Channel2);
		if(!n) {
			GPIO_SetBits(GPIOB, GPIO_Pin_11);
			GPIO_SetBits(GPIOC, GPIO_Pin_4);
			ma_indicator_stm = MA_INDICATOR_STM_OVER;
		}
		else {
			if(time_left(ma_indicator_timer) < 0) {
				printf("error:	timeout\n");
				GPIO_SetBits(GPIOB, GPIO_Pin_11);
				GPIO_SetBits(GPIOC, GPIO_Pin_4);
				ma_indicator_stm = MA_INDICATOR_STM_NULL;
			}
		}
		break;
	case MA_INDICATOR_STM_OVER:
		memcpy(ma_indicator_value, ma_indicator_fifo, sizeof(ma_indicator_fifo));
		ma_indicator_stm = MA_INDICATOR_STM_NULL;
		SPI_Cmd(SPI1, DISABLE);
		DMA_Cmd(DMA1_Channel2, DISABLE);
		Ma_Indicator_Init();
		if(!Ma_Data_Handle(ma_indicator_value, value))
			printf("%s\n", value);
		break;
	}
}

static int cmd_ma_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"  ma set ledr/ledg/ledy 0~100\n"
		"  ma set divice1/divice2 on/off\n"
		"  ma set id xxxxxxxxxx\n"
		"  ma set counter xxxxxxxxxx/+\n"
		"  ma get value/value1/value2\n"
		"  ma get id\n"
		"  ma get counter\n"
		"  ma get button\n"
	};

	if(argc == 3 && !strcmp(argv[1], "get")) {
		if(!strcmp(argv[2], "value1") | !strcmp(argv[2], "value")) {
			if(ma_indicator_stm == MA_INDICATOR_STM_NULL)
				ma_indicator_stm = MA_INDICATOR_STM_INIT_INDICATOR1;
			else {
				printf("operation fail\n");
				return -1;
			}
			return 0;
		}
		else if(!strcmp(argv[2], "value2")) {
			if(ma_indicator_stm == MA_INDICATOR_STM_NULL)
				ma_indicator_stm = MA_INDICATOR_STM_INIT_INDICATOR2;
			else {
				printf("operation fail\n");
				return -1;
			}
			return 0;
		}
		else if(!strcmp(argv[2], "id")) {
			if(Ma_EEPROM_Operation(MA_ID_ADDRESS, ID, 64, 0)) {
				printf("operation fail\n");
				return -1;
			}
			printf("ID = %s\n", ID);
			return 0;
		}
		else if(!strcmp(argv[2], "counter")) {
			if(Ma_EEPROM_Operation(MA_COUNTER_ADDRESS, (unsigned char *)&counter, sizeof(counter), 0)) {
				printf("operation fail\n");
				return -1;
			}
			printf("counter = %d\n", counter);
			return 0;
		}
		else if(!strcmp(argv[2], "button")) {
			printf("%x\n", Ma_Button_Check());
			return 0;
		}
		else {
			printf("error: command is wrong!!\n");
			printf("%s", usage);
			return -1;
		}
	}

	else if(argc == 4 && !strcmp(argv[1], "set")) {
		int a;
		if(!strcmp(argv[2], "ledr")) {
			sscanf(argv[3], "%d", &a);
			if(Ma_LED_Operation(LED_R, a)) {
				printf("operation fail\n");
				return -1;
			}
			printf("operation success\n");
			return 0;
		}
		else if(!strcmp(argv[2], "ledy")) {
			sscanf(argv[3], "%d", &a);
			if(Ma_LED_Operation(LED_Y, a)) {
				printf("operation fail\n");
				return -1;
			}
			printf("operation success\n");
			return 0;
		}
		else if(!strcmp(argv[2], "ledg")) {
			sscanf(argv[3], "%d", &a);
			if(Ma_LED_Operation(LED_G, a)) {
				printf("operation fail\n");
				return -1;
			}
			printf("operation success\n");
			return 0;
		}
		else if(!strcmp(argv[2], "device1")) {
			if(!strcmp(argv[3], "on")) {
				if(Ma_Device_Operation(1, 1)) {
					printf("operation fail\n");
					return -1;
				}
				printf("operation success\n");
				return 0;
			}
			else if(!strcmp(argv[3], "off")) {
				if(Ma_Device_Operation(1, 0)) {
					printf("operation fail\n");
					return -1;
				}
				printf("operation success\n");
				return 0;
			}
			else {
				printf("error: command is wrong!!\n");
				printf("%s", usage);
				return -1;
			}
		}
		else if(!strcmp(argv[2], "device2")) {
			if(!strcmp(argv[3], "on")) {
				if(Ma_Device_Operation(2, 1)) {
					printf("operation fail\n");
					return -1;
				}
				printf("operation success\n");
				return 0;
			}
			else if(!strcmp(argv[3], "off")) {
				if(Ma_Device_Operation(2, 0)) {
					printf("operation fail\n");
					return -1;
				}
				printf("operation success\n");
				return 0;
			}
			else {
				printf("error: command is wrong!!\n");
				printf("%s", usage);
				return -1;
			}
		}
		else if(!strcmp(argv[2], "id")) {
			if(strlen(argv[3]) > 63) {
				printf("operation fail\n");
					return -1;
			}
			strcpy(ID, argv[3]);
			if(Ma_EEPROM_Operation(MA_ID_ADDRESS, ID, strlen(ID) + 1, 1)) {
				printf("operation fail\n");
				return -1;
			}
			printf("operation success\n");
			return 0;
		}
		else if(!strcmp(argv[2], "counter")) {
			if(!strcmp(argv[3], "+")) {
				counter++;
				if(Ma_EEPROM_Operation(MA_COUNTER_ADDRESS, (unsigned char *)&counter, sizeof(counter), 1)) {
					printf("operation fail\n");
					return -1;
				}
				printf("operation success\n");
				return 0;
			}
			else {
				sscanf(argv[3], "%d", &counter);
				if(Ma_EEPROM_Operation(MA_COUNTER_ADDRESS, (unsigned char *)&counter, sizeof(counter), 1)) {
					printf("operation fail\n");
					return -1;
				}
				printf("operation success\n");
				return 0;
			}
		}
		else {
			printf("error: command is wrong!!\n");
			printf("%s", usage);
			return -1;
		}
	}
	else {
		printf("error: command is wrong!!\n");
		printf("%s", usage);
		return -1;
	}
}

const cmd_t cmd_ma = {"ma", cmd_ma_func, "control ma"};
DECLARE_SHELL_CMD(cmd_ma)

static void Ma_Init()
{
	Ma_LED_Init();
	Ma_Device_Init();
	Ma_Button_Init();
	Ma_EEPROM_Init();
	Ma_Indicator_Init();
	if(Ma_EEPROM_Operation(MA_ID_ADDRESS, ID, 64, 0))
		printf("read ID fail\n");
	else
		printf("ID = %s\n", ID);
	if(Ma_EEPROM_Operation(MA_COUNTER_ADDRESS, (unsigned char *)&counter, sizeof(counter), 0))
		printf("read counter fail\n");
	else
		printf("counter = %d\n", counter);
}

static void Ma_Update()
{
	Ma_Indicator_Update();
}

void main()
{
	task_Init();
	Ma_Init();
	while(1) {
		task_Update();
		Ma_Update();
	}
}