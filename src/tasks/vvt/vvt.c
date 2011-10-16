/*
 * miaofng@2009 initial version
 * David@2011
 */
#include <stdio.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "config.h"
#include "sys/task.h"
#include "misfire.h"
#include "time.h"
#include "priv/mcamos.h"
#include "lcm.h"
#include "ad9833.h"
#include "vvt.h"

//for frequence varible
static short ne58x_freq_value;
static short knock_freq_value;
static short wss_freq_value;
static short vss_freq_value;
//for misfire varible
static short misfire_value;
static short misfire_pattern;
//for knock varible
static short knock_pos;
static short knock_width;
static short knock_strength; //unit: mV
static short knock_pattern; //...D C B A
//for advanced gear
static short gear_advance;

//data get from lcm
static lcm_dat_t cfg_data;
static const char lcm_cmd = LCM_CMD_READ;;

//local private
static short vvt_counter; //0-719

//working model
static int vvt_default = 0;

//pravite varibles define
static ad9833_t knock_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PC5,
	.option = AD9833_OPT_OUT_SIN,
};

static ad9833_t vss_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PB0, //real one
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t wss_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PB1,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t rpm_dds = {
	.bus = &spi2,
	.idx = SPI_CS_PB12,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV | AD9833_OPT_SPI_DMA,
};

static void lowlevel_Init(void);
static void dds_Init(void);
static void vss_SetFreq(short hz);
static void wss_SetFreq(short hz);
static void pss_SetSpeed(short hz);
static void knock_SetFreq(short hz);
static void pss_Enable(int on);
static void knock_Enable(int en);
static void pss_SetVolt(pss_ch_t ch, short mv);
static int vvt_GetConfigData(void);

void vvt_Init(void)
{
	time_t overtime_cfg;
	struct mcamos_s lcm = {
		.can = &can1,
		.baud = 500000,
		.id_cmd = MCAMOS_MSG_CMD_ID,
		.id_dat = MCAMOS_MSG_DAT_ID,
		.timeout = 50,
	};

	mcamos_init_ex(&lcm);
	lowlevel_Init();
	dds_Init();
	misfire_Init();

	//wait for lcm initialized
	mdelay(500);
	overtime_cfg = time_get(1000);
	while(vvt_GetConfigData()) {
		if (time_left(overtime_cfg) < 0) {
			vvt_default = 1;
			printf("Simulator Work in Default Status!");
			break;
		}
	}

	vvt_counter = 0;

	if (vvt_default) {
		//for frequence varible
		ne58x_freq_value = 1000;
		wss_freq_value = 100;
		vss_freq_value = 100;
		//for advanced gear
		gear_advance = 0;
		//for misfire varible
		misfire_value = 0;
		//for knock config
		knock_freq_value = 10000;
		knock_strength = 50;
		knock_width = 20;
		knock_pos = 0;
		//for patten
		misfire_pattern = 0;
		knock_pattern = 0;
	} else {
		//for frequence varible
		ne58x_freq_value = cfg_data.rpm;
		wss_freq_value = cfg_data.wss;
		vss_freq_value = cfg_data.vss;
		//for advanced gear
		gear_advance = cfg_data.cam;
		//for misfire varible
		misfire_value = cfg_data.mfr;
		//for knock config
		knock_freq_value = cfg_data.knf;
		knock_pos = cfg_data.knp;
		knock_width = cfg_data.knw;
		knock_strength = cfg_data.knk; //unit: mV
#if 1
		//for patten
		misfire_pattern = 0;
		knock_pattern = 0;
#endif

#if 0
		knock_pattern = (cfg_data.dio >> 8); //...D C B A
		misfire_pattern = cfg_data.dio & 0x00ff;
#endif
	}

	misfire_SetSpeed(ne58x_freq_value? ne58x_freq_value : 1);
	wss_SetFreq(wss_freq_value);
	vss_SetFreq(vss_freq_value);
	knock_SetFreq(knock_freq_value);
	vvt_Start();
}

void vvt_Update(void)
{
	//update lcm input
	if (vvt_GetConfigData() == 0) {
		//mcamos communication indicator
		led_inv(LED_GREEN);

		//frequence init
		if (wss_freq_value != cfg_data.wss)
			wss_SetFreq(cfg_data.wss);
		if (wss_freq_value != cfg_data.vss)
			vss_SetFreq(cfg_data.vss);
		if (wss_freq_value != cfg_data.knk)
			knock_SetFreq(cfg_data.knk);
		if (ne58x_freq_value != cfg_data.rpm)
			misfire_SetSpeed(cfg_data.rpm ? cfg_data.rpm : 1);

		//for frequence varible
		ne58x_freq_value = cfg_data.rpm;
		wss_freq_value = cfg_data.wss;
		vss_freq_value = cfg_data.vss;
		//for advanced gear
		gear_advance = cfg_data.cam;
		//for misfire varible
		misfire_value = cfg_data.mfr;
		//for knock config
		knock_freq_value = cfg_data.knf;
		knock_pos = cfg_data.knp;
		knock_width = cfg_data.knw;
		knock_strength = cfg_data.knk; //unit: mV

#if 0
		misfire_pattern = cfg_data.dio & 0x00ff;
		knock_pattern = (cfg_data.dio >> 8); //...D C B A
#endif
	}
}

void vvt_isr(void)
{
	int mv;
	int x, y, z, tmp;
	int ch, tdc;

	vvt_counter ++;
	if (vvt_counter >= 720)
		vvt_counter = 0;
	if (vvt_counter % 3 == 0) {
		x = vvt_counter/3;	// 0 - 239
		y = (x >> 1) + 1;	// 1 - 120
		z = x & 0x01;

		//output NE58X, pos [58~59] + [58~59]
		mv = IS_IN_RANGE(y, 58, 59);
		mv |= IS_IN_RANGE(y, 118, 119);	//(58+60, 59+60);
		mv = !mv && z;
		pss_SetVolt(NE58X, mv);

		//output CAM1X, pos [56~14]
		mv = IS_IN_RANGE(y, 56, 74);	//14+60;
		pss_SetVolt(CAM1X, !mv);
	}

 	//output CAM4X_IN, pos [22~44] + [52~14] + [22~28] + [52~58]
	tmp = vvt_counter + gear_advance;
	mv = IS_IN_RANGE(tmp, 132, 264);
	mv |= IS_IN_RANGE(tmp, 312, 444);
	mv |= IS_IN_RANGE(tmp, 492, 528);
	mv |= IS_IN_RANGE(tmp, 672, 708);
	pss_SetVolt(CAM4X_IN, !mv);

	//output CAM4X_EXT, pos [12~34] + [42~4] + [12~18] + [42~48]
	tmp = vvt_counter - gear_advance;
	mv = IS_IN_RANGE(tmp, 72, 204);
	mv |= IS_IN_RANGE(tmp, 252, 384);
	mv |= IS_IN_RANGE(tmp, 432, 468);
	mv |= IS_IN_RANGE(tmp, 612, 648);
	pss_SetVolt(CAM4X_EXT, !mv);

	//knock
	for (ch = 0; ch < 4; ch++) {
		mv = (knock_pattern >> ch) & 0x01;
		if(mv != 0) {
			//enable knock signal output
			tdc = 120 + ch * 180;
			tdc += knock_pos;
			mv = IS_IN_RANGE(vvt_counter, tdc, tdc + knock_width) ? 1 : 0;
			knock_Enable(mv);
			if(mv)
				break;
		}
	}

	//misfire
	tmp = misfire_GetSpeed(x);	//x(0,239)
	pss_SetSpeed(tmp);
}

void EXTI9_5_IRQHandler(void)
{
	EXTI->PR = EXTI_Line6;
	vvt_isr();
}

int main(void)
{	
	task_Init();
	vvt_Init();
	while(1) {
		task_Update();
		vvt_Update();
	}
}

/****************************************************************
****************** static local function  ******************
****************************************************************/
static int vvt_GetConfigData(void)
{
	int ret = 0;
	//communication with the lcm board
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, &lcm_cmd, 1);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR + 2, &cfg_data, sizeof(cfg_data));
	if (ret) {
		printf("MCAMOS ERROR!\n");
		return -1;
	}
	return 0;
}

static void lowlevel_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;

	/*knock enable*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,  &GPIO_InitStructure);

	/*pulse output gpio pins*/
	//58X->PIN7, CAM1X->PIN8, CAM4X-IN->PIN9, CAM4X-EXT->PIN8
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*rpm irq init*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource6);
	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	pss_Enable(0);/*misfire input switch*/
}

static void dds_Init(void)
{
	//for rpm dds
	static char rpmdds_rbuf[6];
	static char rpmdds_wbuf[6];
	rpm_dds.p_rbuf = rpmdds_rbuf;
	rpm_dds.p_wbuf = rpmdds_wbuf;

	ad9833_Init(&knock_dds);
	ad9833_Init(&vss_dds);
	ad9833_Init(&wss_dds);
	ad9833_Init(&rpm_dds);
}

static void vss_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&vss_dds, fw);
}

static void wss_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&wss_dds, fw);
}

static void pss_SetSpeed(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&rpm_dds, fw);
}

static void knock_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_KNOCK_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&knock_dds, fw);
}

static void pss_Enable(int on)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = (on > 0) ? ENABLE : DISABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*control the 74lvc1g66,analog switch*/
static void knock_Enable(int en)
{
	if(en)
		GPIO_SetBits(GPIOB, GPIO_Pin_11);
	else
		GPIO_ResetBits(GPIOB, GPIO_Pin_11);
}

static void pss_SetVolt(pss_ch_t ch, short mv)
{
	uint16_t pin;
	BitAction val = Bit_SET;

	if(mv > 0)
		val = Bit_RESET;

	if (ch == CAM4X_EXT) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, val);
	} else {
		pin = 1 << (7 + ch);
		GPIO_WriteBit(GPIOC, pin, val);
	}
}

#if 0
#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vvt.h"
#include "misfire.h"

static int cmd_pss_func(int argc, char *argv[])
{
	int result = -1;
	short speed, ch, mv;
	
	vvt_Stop();
	if(!strcmp(argv[1], "speed") && (argc == 3)) {
		speed = (short)atoi(argv[2]);
		pss_SetSpeed(speed);
		result = 0;
	}
	
	if(!strcmp(argv[1], "volt") && (argc == 4)) {
		ch = (short)atoi(argv[2]);
		mv = (short)atoi(argv[3]);
		ch = (ch > 3) ? 3 : ch;
		pss_SetVolt((pss_ch_t)ch, mv);
		result = 0;
	}
	
	if(result == -1) {
		printf("uasge:\n");
		printf(" pss speed 100\n");
		printf(" pss volt 0 0\n");
	}
	
	return 0;
}

cmd_t cmd_pss = {"pss", cmd_pss_func, "debug pss driver of vvt"};
DECLARE_SHELL_CMD(cmd_pss)

static int cmd_knock_func(int argc, char *argv[])
{
	int result = -1;
	short hz;
	
	vvt_Stop();
	if(!strcmp(argv[1], "freq") && (argc == 3)) {
		hz = (short)atoi(argv[2]);
		knock_SetFreq(hz);
		result = 0;
	}
	
	
	if(result == -1) {
		printf("uasge:\n");
		printf(" knock freq 100\n");
		printf(" knock volt 0 0\n");
	}
	
	return 0;
}

cmd_t cmd_knock = {"knock", cmd_knock_func, "debug knock driver of vvt"};
DECLARE_SHELL_CMD(cmd_knock)

static int cmd_vvt_func(int argc, char *argv[])
{
	int result = -1;
	short speed, s, a, b, c, d, f;
	
	if(!strcmp(argv[1], "speed") && (argc == 3)) {
		speed = (short)atoi(argv[2]);
		misfire_SetSpeed(speed);
		if(speed) vvt_Start();
		else vvt_Stop();
		result = 0;
	}
	
	if(!strcmp(argv[1], "advance") && (argc == 3)) {
		gear_advance = (short)atoi(argv[2]);
		result = 0;
	}
	
	if(!strcmp(argv[1], "misfire") && (argc == 7)) {
		s = (short)(atof(argv[2]) * 32768);
		a = (short) atoi(argv[3]);
		b = (short) atoi(argv[4]);
		c = (short) atoi(argv[5]);
		d = (short) atoi(argv[6]);
		
		a = (a > 0) ? 1 : 0;
		b = (b > 0) ? 2 : 0;
		c = (c > 0) ? 4 : 0;
		d = (d > 0) ? 8 : 0;

		misfire_ConfigStrength(s);
		misfire_ConfigPattern(a+b+c+d);
		result = 0;
	}
	
	if(!strcmp(argv[1], "knock") && (argc == 6)) {
		knock_pos = (short) atoi(argv[2]);
		knock_width = (short) atoi(argv[3]);
		knock_strength = (short) atoi(argv[4]);
		f = (short) atoi(argv[5]);
		knock_SetFreq(f);
		result = 0;
	}
	
	if(!strcmp(argv[1], "kpat") && (argc == 6)) {
		a = (short) atoi(argv[2]);
		b = (short) atoi(argv[3]);
		c = (short) atoi(argv[4]);
		d = (short) atoi(argv[5]);
		
		a = (a > 0) ? 1 : 0;
		b = (b > 0) ? 2 : 0;
		c = (c > 0) ? 4 : 0;
		d = (d > 0) ? 8 : 0;
		knock_pattern = a + b + c + d;
		result = 0;
	}

	if (argv[1][0] == 'v') {
		vss_SetFreq((short)(atoi(argv[2])));
		result = 0;
	}

	if (argv[1][0] == 'w') {
		wss_SetFreq((short)(atoi(argv[2])));
		result = 0;
	}

	if (!strcmp(argv[1], "adc") || (argc == 0)) {
		printf(" adc is: %x, %x, %x, %x, %x \n", vvt_adc[0]&0x0fff, \
										vvt_adc[1]&0x0fff, \
										vvt_adc[2]&0x0fff, \
										vvt_adc[3]&0x0fff, \
										vvt_adc[4]&0x0fff);
		return 1;
	}


	if(result == -1) {
		printf("uasge:\n");
		printf(" vvt speed 100		unit: rpm\n");
		printf(" vvt advance 5		unit: gears, range: 0~10 \n");
		printf(" vvt misfire s A B C D		strength(0.0~1)\n");
		printf(" vvt knock p w s f		pos,width,strength(mV),freq(Hz)\n");
		printf(" vvt kpat A B C D		knock pattern\n");
		printf(" vvt vss 100		unit: rpm\n");
		printf(" vvt wss 100		unit: rpm\n");
		printf(" vvt adc			print adc value\n");
	}
	
	return 0;
}

cmd_t cmd_vvt = {"vvt", cmd_vvt_func, "commands for vvt"};
DECLARE_SHELL_CMD(cmd_vvt)

#endif
