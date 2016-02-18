/*
*  ledcap
*  2016-1-23@miaofng initial version
*  update always poll each ch led status
*
*  API cmds:
*  led <ch_name>	: <ch_name> = A1,A2,A3,A4,B1,B2,B3,B4,C1...
*  return: <+010, {'y':0.0,'r':2.1,'g':0.2}\n\r
*
*  run <ch_name> [y|n| ]	: programing start, ' ' indicates a pulse signal
*  esc <ch_name> [y|n| ]	: cancer programing, ' ' indicates a pulse signal
*  return: <+0\n\r
*
*/

#include "ulp/sys.h"
#include "nvm.h"
#include "led.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"
#include "ulp_time.h"
#include "stm32f10x.h"

static float vref = 3.000f;
static float vthr = 0.8f;
static float vthg = 0.8f;
static float vthy = 0.8f;
static int key_run_ms = 100;
static int key_esc_ms = 100;
static float div_ratio = 0.5;

static uint16_t adc_dma_buf[16];
static uint16_t adc_results[64];

//<ch_name> = a1,a2,a3,a4,b1,b2,b3,b4,c1...
int _bd_(const char *ch_name)
{
	char list[] = "ABCD"; //board_name = <abcd>
	//ch_name = strupr(ch_name);
	char *p = strchr(list, ch_name[0]);
	int index = p - list; //0123.. or negative
	return index;
}

int _ch_(const char *ch_name)
{
	char list[] = "1234"; //ch_name = <1234>
	//ch_name = strupr(ch_name);
	char *p = strchr(list, ch_name[1]);
	int index = p - list; //0123.. or negative
	return index;  //0123.. or negative
}

void gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/*
		pd00-15 A[1-4]#RUN B[1-4]#RUN C[1-4]#RUN D[1-4]#RUN
		pe00-15 A[1-4]#ESC B[1-4]#ESC C[1-4]#ESC D[1-4]#ESC
	*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/*
		pc8	SEL_A
		pc9	SEL_B
	*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void run_set(int bd, int ch, int yes)
{
	int pin = ch;
	pin += bd * 4;
	GPIO_WriteBit(GPIOD, 1 << pin, yes);
}

void esc_set(int bd, int ch, int yes)
{
	int pin = ch;
	pin += bd * 4;
	GPIO_WriteBit(GPIOE, 1 << pin, yes);
}

void bd_set(int bd)
{
	#define SEL_A GPIO_Pin_8
	#define SEL_B GPIO_Pin_9

	switch(bd){
	case 0:
		GPIO_WriteBit(GPIOC, SEL_B, 0);
		GPIO_WriteBit(GPIOC, SEL_A, 0);
		break;
	case 1:
		GPIO_WriteBit(GPIOC, SEL_B, 0);
		GPIO_WriteBit(GPIOC, SEL_A, 1);
		break;
	case 2:
		GPIO_WriteBit(GPIOC, SEL_B, 1);
		GPIO_WriteBit(GPIOC, SEL_A, 0);
		break;
	case 3:
		GPIO_WriteBit(GPIOC, SEL_B, 1);
		GPIO_WriteBit(GPIOC, SEL_A, 1);
		break;
	default:
		break;
	}
}

void adc_init(void)
{
	/*
		pa0-7	ADC00-07
		pb0-1	ADC08-09
		pc0-5	ADC10-15
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	//ADC
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = \
		GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | \
		GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = \
		GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | \
		GPIO_Pin_4 | GPIO_Pin_5 ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	#define ADC1_DR_Address    ((uint32_t)0x4001244C)

	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_dma_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 16;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);

	DMA_Cmd(DMA1_Channel1, ENABLE);

	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	ADC_DeInit(ADC1);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 16;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 5, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 6, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 7, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 8, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 9, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 10, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 11, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 12, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 13, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 14, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 15, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 16, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz

	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
}

void ledcap_init(void)
{
	gpio_init();
	adc_init();

	bd_set(0);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void ledcap_update(void)
{
	static int bd = 0;
	//int eoc = ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC);
	//eoc flag of adc is available only when dma is off
	int eoc = DMA_GetFlagStatus(DMA1_FLAG_TC1);
	if (eoc) {
		DMA_ClearFlag(DMA1_FLAG_TC1);
		uint16_t *p = adc_results;
		p += 16 * bd;
		memcpy(p, adc_dma_buf, sizeof(adc_dma_buf));
		bd ++;
		bd = (bd > 3) ? 0 : bd;
		bd_set(bd);
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	}
}

void ledcap_mdelay(int ms)
{
	time_t deadline = time_get(ms);
	while(time_get(0) < deadline) {
		ledcap_update();
		sys_update();
	}
}

int main(void)
{
	sys_init();
	ledcap_init();
	led_flash(LED_GREEN);
	printf("ledcap v1.0, build: %s %s\n\r", __DATE__, __TIME__);

	while(1) {
		sys_update();
		ledcap_update();
	}
}

#include "shell/cmd.h"
int cmd_run_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"run A1	[y|n|]		start programing board A ch 1\n"
		"	<bd> = A|B|C|D\n"
		"	<ch> = 1|2|3|4\n"
	};

	if(argc > 1) {
		int bd = _bd_(argv[1]);
		int ch = _ch_(argv[1]);
		if(bd >= 0 && ch >= 0) {
			int yes = 1;
			if(argc > 2) {
				yes = (argv[2][0] == 'y') ? 1 : 0;
			}
			run_set(bd, ch, yes);

			if(argc == 2) {
				ledcap_mdelay(key_run_ms);
				run_set(bd, ch, 0);
			}

			printf("<+0\n\r");
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_run = {"run", cmd_run_func, "programing start ctrl"};
DECLARE_SHELL_CMD(cmd_run)

int cmd_esc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"esc B2 [y|n|]		cancel programing board B ch 2\n"
		"	<bd> = A|B|C|D\n"
		"	<ch> = 1|2|3|4\n"
	};

	if(argc > 1) {
		int bd = _bd_(argv[1]);
		int ch = _ch_(argv[1]);
		if(bd >= 0 && ch >= 0) {
			int yes = 1;
			if(argc > 2) {
				yes = (argv[2][0] == 'y') ? 1 : 0;
			}
			esc_set(bd, ch, yes);

			if(argc == 2) {
				ledcap_mdelay(key_esc_ms);
				esc_set(bd, ch, 0);
			}

			printf("<+0\n\r");
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_esc = {"esc", cmd_esc_func, "programing cancel ctrl"};
DECLARE_SHELL_CMD(cmd_esc)

float fabs(float x)
{
	float y = (x > 0.0f) ? x : -x;
	return y;
}

int cmd_led_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"led B2		capture led status of board B ch 2\n"
		"	<bd> = A|B|C|D\n"
		"	<ch> = 1|2|3|4\n"
	};

	if(argc > 1) {
		int bd = _bd_(argv[1]);
		int ch = _ch_(argv[1]);
		if(bd >= 0 && ch >= 0) {
			int dr = adc_results[bd * 16 + ch*4 + 0];
			int dg = adc_results[bd * 16 + ch*4 + 1];
			int dy = adc_results[bd * 16 + ch*4 + 2];
			int d0 = adc_results[bd * 16 + ch*4 + 3];

			float vr = dr * vref / 4096 / div_ratio;
			float vg = dg * vref / 4096 / div_ratio;
			float vy = dy * vref / 4096 / div_ratio;
			float v0 = d0 * vref / 4096 / div_ratio;

			dr = fabs(vr - v0) > vthr ? 1 : 0;
			dg = fabs(vg - v0) > vthg ? 1 : 0;
			dy = fabs(vy - v0) > vthy ? 1 : 0;

			printf("<+%d%d%d, {'y': %.1f, 'r': %.1f, 'g': %.1f, '0': %.1f}\n\r", dy, dr, dg, vy, vr, vg, v0);
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_led = {"led", cmd_led_func, "led status capture"};
DECLARE_SHELL_CMD(cmd_led)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?			to read identification string\n"
		"*RST			instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<ulikar technoledge,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		NVIC_SystemReset();
	}
	else {
		printf("%s", usage);
		return 0;
	}
	return 0;
}
