/*
*
*  miaofng@2017/04/08	initial version for rsu2(hw v4.0)
*
*/
#include <string.h>
#include "stm32f10x.h"
#include "ulp/sys.h"
#include "shell/cmd.h"
#include "common/bitops.h"
#include "can.h"
#include "mbi5025.h"
#include "bsp.h"
#include "pdi.h"

const can_bus_t *bsp_can_bus = &can1;
static const mbi5025_t bsp_mbi = {.bus = &spi1, .load_pin = SPI_1_NSS};
static char bsp_mxc_image[6];

static void bsp_mxc_wimg(void)
{
	mbi5025_write_and_latch(&bsp_mbi, &bsp_mxc_image, sizeof(bsp_mxc_image));
}

static void bsp_mxc_init(void)
{
	memset(bsp_mxc_image, 0x00, sizeof(bsp_mxc_image));
	mbi5025_Init(&bsp_mbi);
	bsp_mxc_wimg();
}

void bsp_mxc_switch(int signal, int on)
{
	if(on & 0x01) bit_set(signal, bsp_mxc_image);
	else bit_clr(signal, bsp_mxc_image);
}

void bsp_relay(int kx, int yes)
{
	const int rly_list[] = {
		-1,
		K_A0, //K1
		K_B0, //K2
		K_C0, //K3
		K_D0, //K4
		K_A4, //K5
		-1, //K6
		K_A1, //K7
		K_B1, //K8
		K_C1, //K9
		K_D1, //K10
		K_B4, //K11
		-1, //K12
		K_A2, //K13
		K_B2, //K14
		K_C2, //K15
		K_D2, //K16
		K_C4, //K17
		-1, //K18
		K_A3, //K19
		K_B3, //K20
		K_C3, //K21
		K_D3, //K22
		K_D4, //K23
		-1, //K24
	};

	if(kx <= 24) {
		switch(kx) {
		case 6:
		case 12:
		case 18:
		case 24: //K_L/R
			bsp_select_rsu(yes);
			break;
		default:
			kx = rly_list[kx];
			if(kx >= 0) {
				bsp_mxc_switch(kx, yes);
				bsp_mxc_wimg();
			}
		}
	}
}

void bsp_led(int mask, int status)
{
	const int led_list[] = {
		LED_LR0, LED_LR1, LED_LR2, LED_LR3,
		LED_RR0, LED_RR1, LED_RR2, LED_RR3,

		LED_LG0, LED_LG1, LED_LG2, LED_LG3,
		LED_RG0, LED_RG1, LED_RG2, LED_RG3,

		LED_LY0, LED_LY1, LED_LY2, LED_LY3,
		LED_RY0, LED_RY1, LED_RY2, LED_RY3,
	};

	for(int i = 0; (mask != 0) && (i < 24); i ++) {
		if(mask & 0x01) {
			bsp_mxc_switch(led_list[i], status & 0x01);
		}

		mask = mask >> 1;
		status = status >> 1;
	}
	bsp_mxc_wimg();
}

void bsp_select_grp(int grp)
{
	int relay_map[5][4] = {
		{K_A0, K_A1, K_A2, K_A3},
		{K_B0, K_B1, K_B2, K_B3},
		{K_C0, K_C1, K_C2, K_C3},
		{K_D0, K_D1, K_D2, K_D3},
		{K_A4, K_B4, K_C4, K_D4},
	};

	if((grp >= 0) && (grp <= 4)) {
		//clear all matrix settings
		for(int j = 0; j < 5; j ++) {
			for(int i = 0; i < 4; i ++) {
				bsp_mxc_switch(relay_map[j][i], 0);
			}
		}

		//enable matrix of selected ecu grp
		for(int i = 0; i < 4; i ++) {
			bsp_mxc_switch(relay_map[grp][i], 1);
		}
		bsp_mxc_wimg();
	}
}

/*
PC5	 GPO MCU_SWBAT_EN
PC6  GPO MCU_BEEPER
PC7  GPO MCU_L/R

PE8  GPO MCU_DG_S0
PE9  GPO MCU_DG_S1
PE14 GPO LED_R
PE15 GPO LED_G

PE12 GPI MCU_RDY_R
PE13 GPI MCU_RDY_L

PA6  GPO SPI1_MISO/MBI_OE
*/

static void bsp_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//MBI_OE
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIOA->BRR = GPIO_Pin_6;

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//about 40Kohm pull up
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

void bsp_swbat(int yes)
{
	if(yes) GPIOC->BSRR = GPIO_Pin_5;
	else GPIOC->BRR = GPIO_Pin_5;
}

void bsp_beep(int yes)
{
	if(yes & 0x01) GPIOC->BSRR = GPIO_Pin_6;
	else GPIOC->BRR = GPIO_Pin_6;
}

//left = 0, right = 1
void bsp_select_rsu(int pos)
{
	if(pos & 0x01) GPIOC->BSRR = GPIO_Pin_7;
	else GPIOC->BRR = GPIO_Pin_7;
}

//ecu = 0 .. 3
void bsp_select_can(int ecu)
{
	//PE8  GPO MCU_DG_S0
	//PE9  GPO MCU_DG_S1
	if(ecu & 0x01) GPIOE->BSRR = GPIO_Pin_8;
	else GPIOE->BRR = GPIO_Pin_8;

	if(ecu & 0x02) GPIOE->BSRR = GPIO_Pin_9;
	else GPIOE->BRR = GPIO_Pin_9;
}

//pos: 0=left, 1=right, return ready(pushed) or not
int bsp_rdy_status(int pos)
{
	//PE13 GPI MCU_RDY_L
	//PE12 GPI MCU_RDY_R
	int ready;
	if(pos == 0) ready = (GPIOE->IDR & GPIO_Pin_13) ? 0 : 1;
	else ready = (GPIOE->IDR & GPIO_Pin_12) ? 0 : 1;
	return ready;
}

/*
PA0/ADC123_IN0	MCU_PT204_L0
PA1/ADC123_IN1	MCU_PT204_L1
PA2/ADC123_IN2	MCU_PT204_L2
PA3/ADC123_IN3	MCU_PT204_L3

PC0/ADC123_IN10	MCU_PT204_R0
PC1/ADC123_IN11	MCU_PT204_R1
PC2/ADC123_IN12	MCU_PT204_R2
PC3/ADC123_IN13	MCU_PT204_R3

PC4/ADC12_IN14	MCU_SWBAT_FB
*/
static void bsp_adc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

#if (CONFIG_RSU_P71A == 1) || (CONFIG_RSU_KP108 == 1)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
#else
	//use stm32 gpio internal weak-up, about 40-50KOHM
	//no need to mount on board RP1 & RP6 now!!!
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
#endif
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//ADC1 MCU_PT204_L0~3
	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 4); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	//ADC2 MCU_PT204_R0~3, MCU_SWBAT_FB
	ADC_DeInit(ADC2);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC2, ADC_Channel_14, 1, ADC_SampleTime_239Cycles5 );

	ADC_InjectedSequencerLengthConfig(ADC2, 4); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_11, 2, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_12, 3, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_13, 4, ADC_SampleTime_239Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC2, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC2, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC2, ENABLE);
	ADC_ResetCalibration(ADC2);
	while(ADC_GetResetCalibrationStatus(ADC2));
	ADC_StartCalibration(ADC2);
	while(ADC_GetCalibrationStatus(ADC2)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC2, ENABLE);
}

//left pos = 0, right pos = 1, mV = NULL if not to be used
//return bit map of rsu exist or not
int bsp_rsu_status(int pos, int *mV)
{
	//note: msb is sign bit, always 0
	#if CONFIG_RSU4_NO_LM4040 == 1
	#define BSP_PT204_D2MV(d)  ((3300 * d) >> 15)
	#else
	#define BSP_PT204_D2MV(d)  ((3000 * d) >> 15)
	#endif

	int mv[4];
	if(pos == 0) {
		mv[0] = BSP_PT204_D2MV(ADC1->JDR1);
		mv[1] = BSP_PT204_D2MV(ADC1->JDR2);
		mv[2] = BSP_PT204_D2MV(ADC1->JDR3);
		mv[3] = BSP_PT204_D2MV(ADC1->JDR4);
	}
	else {
		mv[0] = BSP_PT204_D2MV(ADC2->JDR1);
		mv[1] = BSP_PT204_D2MV(ADC2->JDR2);
		mv[2] = BSP_PT204_D2MV(ADC2->JDR3);
		mv[3] = BSP_PT204_D2MV(ADC2->JDR4);
	}

	if(mV != NULL) {
		memcpy(mV, mv, sizeof(mv));
	}

	/*threshold voltage when pt204 is in dark
	2669 2614 2421 2525 n-uut n-push @50KOHM
	2702 2660 2898 2888 y-uut n-push @50KOHM
	2638 2657 2999 2999 y-uut y-push @50KOHM
	2638 2658 2999 2999 y-uut y-push @50KOHM
	2651 2668 2999 2999 y-uut y-push @50KOHM
	*/
	#if CONFIG_RSU4_NO_LM4040 == 1
	const int vth = 3100; //unit: mV(range: 2.999-3.300)
	#else
	const int vth = 2998; //unit: mV(range: 2.999-3.300)
	#endif

	int status = 0;
	for(int i = 0; i < 4; i ++) {
		status |= (mv[i] > vth) ? (1 << i) : 0;
	}

	#if CONFIG_RSU4_WITH_PROBE == 1
	status = ~status; //no rsu => 3v3(contrary to light sensor)
	#endif

	return status;
}

int bsp_vbat(void)
{
	//10K 1K
	#define BSP_VBAT_D2MV(d)  ((11 * 3000 * d) >> 16)
	int d, mv;
	d = ADC2->DR;
	mv = BSP_VBAT_D2MV(d);
	return mv;
}

void bsp_init(void)
{
	bsp_gpio_init();
	bsp_adc_init();
	bsp_mxc_init();
}

void bsp_reset(void)
{
	NVIC_SystemReset();
}

#if 1
static int cmd_bsp_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"bsp mxc 0-47 0/1	mbi5024 output ctrl\n"
		"bsp rly 1-23 0/1	mbi5024 rly Kxx setting\n"
		"bsp led img		img=0x00YYGGRR right|left\n"
		"bsp beep [0/1]		beeper on/off\n"
		"bsp adc [pos]		rsu status, pos=0/1\n"
		"bsp vbat [0/1]		vbat set & measure\n"
		"bsp rsu [0..1]		left/right fixture rsu sel\n"
		"bsp can [0..3]		can ecu sel\n"
		"bsp grp [0..4]		matrix ecu group sel\n"
		"bsp rdy			probe switch status\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "mxc")) {
		int sig = (argc >= 3) ? atoi(argv[2]) : 0;
		int lvl = (argc >= 4) ? atoi(argv[3]) : 1;
		bsp_mxc_switch(sig, lvl);
		bsp_mxc_wimg();
	}
	else if (!strcmp(argv[1], "rly")) {
		int kxx = (argc >= 3) ? atoi(argv[2]) : 0;
		int yes = (argc >= 4) ? atoi(argv[3]) : 1;
		bsp_relay(kxx, yes);
		bsp_mxc_wimg();
	}
	else if (!strcmp(argv[1], "led")) {
		int msk = 0x00FFFFFF;
		int img = 0;

		if(argc >= 3){
			if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &img);
			else sscanf(argv[2], "%d", &img);
		}
		if(argc >= 4){
			if(!strncmp(argv[3], "0x", 2)) sscanf(argv[3], "0x%x", &msk);
			else sscanf(argv[3], "%d", &msk);
		}

		bsp_led(msk, img);
	}
	else if (!strcmp(argv[1], "beep")) {
		int yes = (argc >= 3) ? atoi(argv[2]) : 1;
		bsp_beep(yes);
	}
	else if (!strcmp(argv[1], "adc")) {
		int pos = (argc >= 3) ? atoi(argv[2]) : 0;
		int mv[4];

		int status = bsp_rsu_status(pos, mv);
		printf("rsu status = 0x%02x[0:%04d 1:%04d 2:%04d 3:%04d]\n", status, mv[0], mv[1], mv[2], mv[3]);
	}
	else if (!strcmp(argv[1], "vbat")) {
		if(argc >= 3) {
			int yes = (argc >= 3) ? atoi(argv[2]) : 1;
			bsp_swbat(yes);
		}
		else {
			int mv = bsp_vbat();
			printf("vbat = %04d mV\n", mv);
		}
	}
	else if (!strcmp(argv[1], "rsu")) {
		int pos = (argc >= 3) ? atoi(argv[2]) : 0;
		bsp_select_rsu(pos);
	}
	else if (!strcmp(argv[1], "can")) {
		int ecu = (argc >= 3) ? atoi(argv[2]) : 0;
		bsp_select_can(ecu);
	}
	else if (!strcmp(argv[1], "grp")) {
		int grp = (argc >= 3) ? atoi(argv[2]) : 0;
		bsp_select_grp(grp);
	}
	else if (!strcmp(argv[1], "rdy")) {
		int rdy_l = bsp_rdy_status(0);
		int rdy_r = bsp_rdy_status(1);
		printf("ready for test: %d@left, %d@right\n", rdy_l, rdy_r);
	}

	return 0;
}

const cmd_t cmd_bsp = {"bsp", cmd_bsp_func, "bsp debug cmds"};
DECLARE_SHELL_CMD(cmd_bsp)
#endif

void board_reset(void)
{
	NVIC_SystemReset();
}

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<ULICAR Technology,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		board_reset();
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}

	return 0;
}
