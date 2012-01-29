/* yarn break sensor
 * design concept:
 * 	1, powerup (or triggered by reset key) calibration(to calcu Gs and Vs0)
 * 	2, read-dsp-write loop updated by hw timer isr (Vo = Voref + G/Gs/Go * (Vs - Vs0))
 * name conventions:
 * 	vr, reference voltage, output by ch1 of ad5663
 * 	vs, the "in+" of opa, which is the very weak stress sensor voltage
 * 	vi, opa output, that is mcu adc input, digital or analog voltage
 * 	vo, output by ch2 of ad5663(range: 0~2v5), or ybs output(range: 0-24v)
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */

#include <stdio.h>
#include "config.h"
#include "sys/task.h"
#include "stm32f10x.h"
#include "ulp/device.h"
#include "ulp/dac.h"
#include "ybs_drv.h"

void ybs_mdelay(int ms)
{
	time_t timer = time_get(ms);
	while(time_left(timer) > 0) {
		task_Update();
	}
}

static void ybs_init(void)
{
	ybs_drv_init();
}

/*calibrate the opa offset & gain:
 * input: Vs(stress sensor output) , Vofs(offset voltage, output by DAC CH1)
 * output: D(ADC read back)
 * equation: Vs * Gs + Vofs * Gofs = D (note: according schematic, Gs = Gofs + 1)
 * solution:
 * 	1, Vs0 * Gs + Vofs0 *Gofs = Vi0
 * 	2, Vs0 * Gs + Vofs1 *Gofs = Vi1
 * 	3, eq2-eq1 => (Vofs1 - Vofs0) * Gofs = Vi1 - Vi0 => then we can got  Gs and Vs
 */
static void ybs_cal(void)
{
	int vi, vr, ve, vl, vh, i, vm;
	int vr1, vr2, vi1, vi2, gs;

	//test vr = 0
	vr = v2d(0.0);
	ve = v2d(3.0);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean(); //got it
	if(vi < ve) {
		printf("fail: vr = %dmv, vi = %dmv(<%dmv!)\n", d2mv(vr), d2mv(vi), d2mv(ve));
		return -1;
	}

	//test vr = 2v5
	vr = v2d(2.5);
	ve = v2d(0.3);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean(); //got it
	if(vi > ve) {
		printf("fail: vr = %dmv, vi = %dmv(>%dmv!)\n", d2mv(vr), d2mv(vi), d2mv(ve));
		return -1;
	}

	//search the mid point of Vref where vi = 2.5v(range 2.5v->0.5v, best for OPA)
	vm = v2d(2.5);
	vl = v2d(0.0);
	vh = v2d(3.3);
	for(i = 0; i < 24; i ++) { //to try 24 times, can break in advance, but not necessary
		vr = (vl + vh) / 2;
		ybs_set_vr(vr);
		ybs_mdelay(10);
		vi = ybs_get_vi_mean();
		if(vi < vm) { //opa output too low, vref should down
			vh = vr;
		}
		else { //vref should up
			vl = vr;
		}

		//display
		printf("cal %02d: vr = %08duv, vi = %04dmv\n", i, d2uv(vr), d2mv(vi));
	}

	vr1 = vr;
	vi1 = vi;

	//search the mid point of Vref where vi = 0.5v(range 2.5v->0.5v, best for OPA)
	vm = v2d(0.5);
	vl = v2d(0.0);
	vh = v2d(3.3);
	for(i = 0; i < 24; i ++) { //to try 24 times, can break in advance, but not necessary
		vr = (vl + vh) / 2;
		ybs_set_vr(vr);
		ybs_mdelay(10);
		vi = ybs_get_vi_mean();
		if(vi < vm) { //opa output too low, vref should down
			vh = vr;
		}
		else { //vref should up
			vl = vr;
		}

		//display
		printf("cal %02d: vr = %08duv, vi = %04dmv\n", i, d2uv(vr), d2mv(vi));
	}

	vr2 = vr;
	vi2 = vi;

	//to calculate the gain, gs = 1 - gr
	gs = 1 - (vi2 - vi1) / (vr2 - vr1);

	/*
	 * note: gain of stress sensor will be changed with the static dc resitance and  its pull down resistor,
	 * it's neglected dueto the very weak influence in a real system.
	 */
	return gs;
}

/*read-dsp-write loop*/
void ybs_update(void)
{
}

void main(void)
{
	task_Init();
	ybs_init();

	printf("Power Conditioning - YBS\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	ybs_cal();

	while(1){
		task_Update();
		ybs_Update();
		//gpcon_signal(SENSOR,SENPR_ON);
		//ybs_mdelay(5000);
		//gpcon_signal(SENSOR,SENPR_OFF);
		//ybs_mdelay(5000);
	}
} // main

static int adc_value;
static int dac_value;
static int fd;
static char buf[6];

int adc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/8 = 9Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//PA1 analog voltage input, ADC12_IN1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  //Single MODE
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5); //9Mhz/(239.5 + 12.5) = 35.7Khz
	//ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, ENABLE);

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	return 0;
}

int timer_init(int sample_fz)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = sample_fz;
	TIM_TimeBaseStructure.TIM_Prescaler = 16 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	//TIM_ARRPreloadConfig(TIM4, ENABLE);

	TIM_Cmd(TIM4, ENABLE);

	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	return 0;
}


void TIM4_IRQHandler(void)
{
	//printf("adc value is %d\n",ADC_GetConversionValue(ADC1));
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_Cmd(TIM4, DISABLE);
	adc_value = ADC_GetConversionValue(ADC1);
	printf("\nADC convert result:%d\n",adc_value);
	printf("DAC input value   :%d\n",dac_value);
	if(adc_value < 2000){
		printf("ADC convert result:%d\n",adc_value);
		printf("DAC input value   :%d\n",dac_value);
		TIM_Cmd(TIM4, DISABLE);
	}
	dac_value += 2;
	if(dac_value > 65535)
		dac_value = 0;
	sprintf(buf,"%d",dac_value);
	dev_ioctl(fd,DAC_CHOOSE_CH,0);
	dev_write(fd,buf,0);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

