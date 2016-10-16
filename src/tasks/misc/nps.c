/*
 * 	feng.ni@2016-10-16  rewrite & simplify
 */

#include "ulp/sys.h"
#include "shell/cmd.h"
#include "stm32f10x.h"
#include "led.h"
#include "dac.h"
#include <stdlib.h>
#include <string.h>

void nps_init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/*v1.0 pinmap
	PB1 ADC 32V_PB1_AD Vboost_mic2287 * 10K / (128K + 10K)
	PC0 ADC ADC_PC0 Iblow INA193*0R02
	PC2 ADC OPA_PC2_AD OPA170_VOUT * 1K/(1K + 10K)

	PA4 DAC DAC1_PA4 dac_out

	PA0 GPO ZXGD_PA0 RELAY@MMUN2211
	PC6 GPO OUTPUT_PC6 MOS DISCHARGE
	PC3 GPO LED_R
	PE5 GPO LED_G
	*/

	/*v1.1 pinmap, !!!means changed towards v1.0
	PB1 ADC VDD_35V_FB XL6008 * 1K / (27K + 1K)
	PC0 ADC IS_FB INA193*0R2
	PC2 ADC OPA_OUT OPA551_VOUT * 2K/(2K + 18K)

	PA4 DAC OPA_IN dac_out

	PA0 GPO NPS_SW@MMUN2211
	PC6 GPI OPA_FAIL !!!!
	PE0 GPO LED_R !!!
	PE1 GPO LED_G !!!
	*/

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	//ADC
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//GPO
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

#if CONFIG_HW_V1P0 == 1
	//PC3 GPO LED_R
	//PE5 GPO LED_G
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#else
	//PE0 GPO LED_R !!!
	//PE1 GPO LED_G !!!
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

	//TIM
	TIM_TimeBaseStructure.TIM_Period = 1000; //to be changed later
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; //Ftick = 1MHz
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	//DAC
	dac_ch1.init(NULL);
}

void nps_udelay(float uS)
{
	uint16_t T = (int) uS;
	T = (T == 0) ? 1 : T;

	TIM_Cmd(TIM2, DISABLE);
	TIM2->ARR = T;
	TIM2->CNT = 0;
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_Cmd(TIM2, ENABLE);
	while(1) {
		FlagStatus overflow = TIM_GetFlagStatus(TIM2, TIM_FLAG_Update);
		if(overflow == SET)
			break;
	}
}

void nps_mdelay(float ms)
{
	ms += 0.999;
	time_t deadline = time_get((int) ms);
	while(time_left(deadline) > 0) {
		sys_update();
	}
}

void nps_enable(int yes)
{
	BitAction ba = (yes) ? Bit_SET : Bit_RESET;
	GPIO_WriteBit(GPIOA, GPIO_Pin_0, ba);
	nps_mdelay(10);
}

int nps_vcc_set(float vcc)
{
	float gain, vofs;
	float vref = 2.500; //unit: V
	float vdac = 0;
	int ddac = 0;

#if CONFIG_HW_V1P0 == 1
	// gain = 14K/1K OPA170
	// vofs = 2.5V * 3K / (22K + 3K) = 0.300V
	gain = 14.00;
	vofs = 0.300;
#else
	//gain = 14K/1K OPA551
	//vofs = 2.5V * 1K / (6.8K + 1K) = 0.320V
	gain = 14.00;
	vofs = 0.320;
#endif

	vdac = vcc / gain + vofs;
	vdac = vdac / vref * 4096;

	ddac = (int) vdac;
	ddac = (ddac < 0) ? 0 : ddac;
	ddac = (ddac > 4095) ? 4095 : ddac;

	dac_ch1.write(ddac);
	return 0;
}

#define _keycode(key, code) (key << 8 | code)
#define _key(keycode) ((keycode >> 8) & 0xff)
#define _code(keycode) (keycode & 0xff)

enum {
	FUSE_TEST = _keycode(0, 1),
	POL_HIGH = _keycode(2, 8), //opposite tooth polarity = high
	FASTSWAP = _keycode(2, 16), //switch transition = single step
	SWPT_50 = _keycode(2, 32), //switchpoints = 50%
	HYSMD_CONV = _keycode(2, 64), //hysteresis = conventional
	OSR_SLOW = _keycode(2, 128), //output slow rate = slow
	LOCK = _keycode(3, 128), //global lock setting

	TPOS_TRIM = _keycode(3, 0), //0-63
	TPOS_TRIM_AUTO = _keycode(6, 0),

	BASE_TRIM = _keycode(1, 0), //0-255
	BASE_TRIM_AUTO = _keycode(7, 0),
};

int nps_write(int keycode, int blow)
{
	int key = _key(keycode);
	int code = _code(keycode);
	float blow_ms = 0;
	float pulw_us = 0;

	switch(keycode) {
	case TPOS_TRIM_AUTO:
		blow_ms = 4.0;
		break;
	case BASE_TRIM_AUTO:
		blow_ms = 7.5;
		break;
	default:
		blow_ms = 0.1;
	}

	#define vpl 05.0 //low voltage during programming
	#define vpm 15.0 //mid voltage during programming
	#define vph 29.0 //high voltage during programming

	blow_ms = (blow) ? blow_ms * 2.5 : 0; //min * 2.5
#if CONFIG_HW_V1P0 == 1
	pulw_us = 90; //pulse width
#else
	//slow down to solve pulse slewrate issue
	pulw_us = 200; //pulse width
#endif

	//reset last write cycle
	nps_vcc_set(0.0);
	nps_mdelay(100.0); //interframe delay

	//current write cycle start
	nps_vcc_set(vpl);
	nps_mdelay(1.0);

	//write key
	nps_vcc_set(vph);
	nps_udelay(pulw_us);
	nps_vcc_set(vpl);
	nps_udelay(pulw_us);
	for(int i = 0; i < key; i ++) {
		nps_vcc_set(vpm);
		nps_udelay(pulw_us);
		nps_vcc_set(vpl);
		nps_udelay(pulw_us);
	}

	//write code
	nps_vcc_set(vph);
	nps_udelay(pulw_us);
	nps_vcc_set(vpl);
	nps_udelay(pulw_us);
	for(int i = 0; i < code; i ++) {
		nps_vcc_set(vpm);
		nps_udelay(pulw_us);
		nps_vcc_set(vpl);
		nps_udelay(pulw_us);
	}

	//blow?
	if(blow_ms > 0.0) {
		nps_vcc_set(vph);
		nps_mdelay(blow_ms);
		nps_vcc_set(vpl);
	}

	nps_mdelay(1.0);
	return 0;
}

int nps_step(int step){
	int ecode = -1;
	switch(step) {
	case 1:
		ecode = nps_write(BASE_TRIM_AUTO, 1);
		break;

	case 2: //2 6 4 9
		ecode = nps_write(TPOS_TRIM_AUTO, 1);
		ecode += nps_write(POL_HIGH, 1);
		ecode += nps_write(FASTSWAP, 1);
		ecode += nps_write(LOCK, 1);
		break;
	}

	return ecode;
}

void main(void)
{
	nps_init();
	sys_init();
	while(1) {
		sys_update();
	}
}

static int cmd_nps_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"allegro relay on		enable allegro signal link\n"
		"allegro step n		customized labview api\n"
		"allegro burn key val [!]		key code try set, add ! to burn\n"
		"allegro dac 1.00		dac output debug, unit: V\n"
	};

	int ecode = -1;
	int n, step, yes, key, code;
	float v;

	if(argc < 3) {
		printf("%s", usage);
		return 0;
	}

	switch(argv[1][0]) {
	case 'b': //burn
		if(argc >= 4) {
			n = sscanf(argv[2], "%d", &key);
			n += sscanf(argv[3], "%d", &code);
			if(n == 2) {
				yes = (argc == 5) && (argv[4][0] == '!');
				ecode = nps_write(_keycode(key, code), yes);
			}
		}
		break;

	case 'd': //dac
		n = sscanf(argv[2], "%f", &v);
		if(n == 1) {
			ecode = nps_vcc_set(v);
		}
		break;

	case 'r': //relay
	case 'R':
		yes = strcmp(argv[1], "on") == 0;
		nps_enable(yes);
		ecode = 0;
		break;

	case 's': //step
		n = sscanf(argv[2], "%d", &step);
		if(n > 0) {
			ecode = nps_step(step);
		}
		break;

	default:
		break;
	}

	if(ecode) printf("<%d, operation failed!\n", ecode);
	else printf("OK\n");
	return 0;
}

cmd_t cmd_allegro = {"allegro", cmd_nps_func, "commands for allegro programing"};
DECLARE_SHELL_CMD(cmd_allegro)
