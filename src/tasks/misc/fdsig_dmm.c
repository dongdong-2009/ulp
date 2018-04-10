/*
*
* miaofng@2018-01-31 routine for ford online hub tester
* 1, fdsig includes: mux, audio and dmm <optional, in case of external dmm not used>
* 2, mux is like "hotkey" of cmd cmd gpio
*
* miaofng@2018-04-10 update to support fdsig v1.1
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "common/fifo.h"
#include "stm32f10x.h"
#include "gpio.h"
#include "pwm.h"
#include "tm770x.h"
#include "nvm.h"
#include "fdsig.h"

/*
current calibration coefficient, note: bank & ch calibration will be
merged together(into dmm_cal) during channel switch
*/
static dmm_cal_t dmm_cal;
const dmm_ch_t *dmm_ch = NULL; //current selected channel

#if CONFIG_FDPLC_V1P0 == 1
const dmm_ch_t dmm_ch_list[] = {
	{.dg408=0x00, .dg409=BANK_18V, .ainx=TM_AIN1, .gain=1, .name="OFF"},
	//DG408#1
	{.dg408=0x10, .dg409=BANK_18V, .ainx=TM_AIN1, .gain=1, .name="VBAT+"},
	{.dg408=0x11, .dg409=BANK_18V, .ainx=TM_AIN1, .gain=1, .name="VBAT-"},
	{.dg408=0x12, .dg409=BANK_18V, .ainx=TM_AIN1, .gain=1, .name="VDIM"},
	{.dg408=0x13, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VUSB1"},
	{.dg408=0x14, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VUSB3C"},
	{.dg408=0x15, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VUSB2C"},
	{.dg408=0x16, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VUSB3"},
	{.dg408=0x17, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VUSB2"},

	//DG408#2
	{.dg408=0x21, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VIREF"},
	{.dg408=0x23, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VREF_3V0"},
	{.dg408=0x24, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VIBAT"},
	{.dg408=0x25, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VIELOAD1"},
	{.dg408=0x26, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VIELOAD2"},
	{.dg408=0x27, .dg409=BANK_06V, .ainx=TM_AIN1, .gain=1, .name="VIELOAD3"},

	//DIAG@CD4051
	{.dg408=0x20, .dg409=BANK_06V, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="VDIAG"},
	{.dg408=0x20, .dg409=BANK_06V, .diagx=1, .ainx=TM_AIN1, .gain=1, .name="VAUXL"},
	{.dg408=0x20, .dg409=BANK_06V, .diagx=2, .ainx=TM_AIN1, .gain=1, .name="VAUXR"},
	{.dg408=0x20, .dg409=BANK_06V, .diagx=3, .ainx=TM_AIN1, .gain=1, .name="VAUXD"},
	{.dg408=0x20, .dg409=BANK_06V, .diagx=4, .ainx=TM_AIN1, .gain=1, .name="VAUXG"},
	{.dg408=0x20, .dg409=BANK_06V, .diagx=5, .ainx=TM_AIN1, .gain=1, .name="VOTG"},

	//CDP
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=0, .ainx=TM_AIN1, .gain=1, .name="VREF_0V6"},
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=1, .ainx=TM_AIN1, .gain=1, .name="VCDP1"},
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=2, .ainx=TM_AIN1, .gain=1, .name="VCDP2"},
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=3, .ainx=TM_AIN1, .gain=1, .name="VCDP3"},
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=4, .ainx=TM_AIN1, .gain=1, .name="VCDP2C"},
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=5, .ainx=TM_AIN1, .gain=1, .name="VCDP3C"},
	{.dg408=0x22, .dg409=BANK_06V, .vcdpx=6, .ainx=TM_AIN1, .gain=1, .name="VOFS_2V5"},

	//virtual channels
	{.dg408=0x11, .dg409=BANK_1V5, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="IQBAT"}, //VBAT-
	{.dg408=0x11, .dg409=BANK_1V5, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="I0BAT"}, //VBAT-
	{.dg408=0x24, .dg409=BANK_ACS, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="IBAT"}, //VIBAT
	{.dg408=0x25, .dg409=BANK_ACS, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="IELOAD1"}, //VIELOAD1
	{.dg408=0x26, .dg409=BANK_ACS, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="IELOAD2"}, //VIELOAD2
	{.dg408=0x27, .dg409=BANK_ACS, .diagx=0, .ainx=TM_AIN1, .gain=1, .name="IELOAD3"}, //VIELOAD3
};
#else
const dmm_ch_t dmm_ch_list[] = {
	{.dg408=0x00, .gain=1, .ainx=TM_AIN1,  .name="OFF"},

	//DG408#1
	{.dg408=0x10, .gain=1, .ainx=TM_AIN1, .name="VBAT+"},
	{.dg408=0x11, .gain=1, .ainx=TM_AIN1, .name="VBAT-"},
	{.dg408=0x12, .gain=1, .ainx=TM_AIN1, .name="VUSB1"},
	{.dg408=0x13, .gain=1, .ainx=TM_AIN1, .name="VUSB2"},
	{.dg408=0x14, .gain=1, .ainx=TM_AIN1, .name="VUSB3"},
	{.dg408=0x15, .gain=1, .ainx=TM_AIN1, .name="VUSB2C"},
	{.dg408=0x16, .gain=1, .ainx=TM_AIN1, .name="VUSB3C"},
	{.dg408=0x17, .gain=1, .ainx=TM_AIN1, .name="VREF_3V0"},

	{.dg408=0x11, .gain=1, .ainx=TM_AIN1, .name="IQBAT"}, //VBAT-
	{.dg408=0x11, .gain=1, .ainx=TM_AIN1, .name="I0BAT"}, //VBAT-

	//DG408#2
	{.dg408=0x20, .gain=1, .ainx=TM_AIN1, .name="VCDP"},
	{.dg408=0x21, .gain=1, .ainx=TM_AIN1, .name="VREF_IS"}, //VIREF
	{.dg408=0x22, .gain=1, .ainx=TM_AIN1, .name="VDIAG"},
	{.dg408=0x23, .gain=1, .ainx=TM_AIN1, .name="VDIM"},
	{.dg408=0x24, .gain=1, .ainx=TM_AIN1, .name="VIBAT"},
	{.dg408=0x25, .gain=1, .ainx=TM_AIN1, .name="VIELOAD1"},
	{.dg408=0x26, .gain=1, .ainx=TM_AIN1, .name="VIELOAD2"},
	{.dg408=0x27, .gain=1, .ainx=TM_AIN1, .name="VIELOAD3"},
	//unit: A
	{.dg408=0x24, .gain=1, .ainx=TM_AIN1, .name="IBAT"},
	{.dg408=0x25, .gain=1, .ainx=TM_AIN1, .name="IELOAD1"},
	{.dg408=0x26, .gain=1, .ainx=TM_AIN1, .name="IELOAD2"},
	{.dg408=0x27, .gain=1, .ainx=TM_AIN1, .name="IELOAD3"},

	//CDP
	{.dg408=0x20, .vcdpx=0, .gain=1, .ainx=TM_AIN1, .name="VREF_0V6"},
	{.dg408=0x20, .vcdpx=1, .gain=1, .ainx=TM_AIN1, .name="VCDP1"},
	{.dg408=0x20, .vcdpx=2, .gain=1, .ainx=TM_AIN1, .name="VCDP2"},
	{.dg408=0x20, .vcdpx=3, .gain=1, .ainx=TM_AIN1, .name="VCDP3"},
	{.dg408=0x20, .vcdpx=4, .gain=1, .ainx=TM_AIN1, .name="VCDP2C"},
	{.dg408=0x20, .vcdpx=5, .gain=1, .ainx=TM_AIN1, .name="VCDP3C"},
	{.dg408=0x20, .vcdpx=6, .gain=1, .ainx=TM_AIN1, .name="N.A."},
	{.dg408=0x20, .vcdpx=7, .gain=1, .ainx=TM_AIN1, .name="GND"},

	//DIAG@CD4051
	{.dg408=0x22, .diagx=0, .gain=1, .ainx=TM_AIN1, .name="VIREF"},
	{.dg408=0x22, .diagx=1, .gain=1, .ainx=TM_AIN1, .name="VAUXL"}, //MIC_L
	{.dg408=0x22, .diagx=2, .gain=1, .ainx=TM_AIN1, .name="VAUXR"}, //MIC_R
	{.dg408=0x22, .diagx=3, .gain=1, .ainx=TM_AIN1, .name="VAUXD"}, //MIC_DET
	{.dg408=0x22, .diagx=4, .gain=1, .ainx=TM_AIN1, .name="VAUXG"}, //MIC_GND
	{.dg408=0x22, .diagx=5, .gain=1, .ainx=TM_AIN1, .name="VOTG"}, //OTG_ID
	//unit: OHM
	{.dg408=0x22, .diagx=0, .gain=1, .ainx=TM_AIN1, .name="RIREF"},
	{.dg408=0x22, .diagx=1, .gain=1, .ainx=TM_AIN1, .name="RAUXL"}, //MIC_L
	{.dg408=0x22, .diagx=2, .gain=1, .ainx=TM_AIN1, .name="RAUXR"}, //MIC_R
	{.dg408=0x22, .diagx=3, .gain=1, .ainx=TM_AIN1, .name="RAUXD"}, //MIC_DET
	{.dg408=0x22, .diagx=4, .gain=1, .ainx=TM_AIN1, .name="RAUXG"}, //MIC_GND
	{.dg408=0x22, .diagx=5, .gain=1, .ainx=TM_AIN1, .name="ROTG"}, //OTG_ID
};
#endif

static const dmm_cal_t dmm_cal_rom[] = {
	#if CONFIG_DMM_INT == 1
		//dmm_v = g0 * vadc + d0, bank calibration
		#if CONFIG_FDPLC_V1P0 == 1
		{.d = +0.0, .g = 108.2/8.200, .unit = "V", .name = "BANK_18V"}, //BANK@BANK_18V, 8K2|100K
		{.d = +0.0, .g = 133.0/33.00, .unit = "V", .name = "BANK_06V"}, //BANK@BANK_06V, 33K|100K
		{.d = +2.5, .g = 200.0/100.0, .unit = "V", .name = "BANK_ACS"}, //BANK@BANK_ACS, 100K|100K
		{.d = +0.0, .g = 100.0/100.0, .unit = "V", .name = "BANK_1V5"}, //BANK@BANK_1V5
		#else
		{.d = DMM_VOFS, .g = DMM_GAIN, .unit = "V", .name = "BANK_ANY"},
		#endif
	#endif

	//dmm_i = (dmm_v - d1) * g1, ch calibration
	{.d = 0.00000, .g = -1.0/IBAT_RS, .unit = "A", .name = "IQBAT"}, //VBAT-
	{.d = 0.00000, .g = -1.0/IBAT_RS, .unit = "A", .name = "I0BAT"}, //VBAT-
	{.d = ACS_OFS, .g = +1.0/ACS_G20, .unit = "A", .name = "IBAT"},
	{.d = ACS_OFS, .g = +1.0/ACS_G20, .unit = "A", .name = "IELOAD1"},
	{.d = ACS_OFS, .g = +1.0/ACS_G20, .unit = "A", .name = "IELOAD2"},
	{.d = ACS_OFS, .g = +1.0/ACS_G20, .unit = "A", .name = "IELOAD3"},

	//ohm = (dmm_v - d1) * g1
	{.d = OHM_ROFS, .g = +1.0/OHM_IREF, .unit = "R", .name = "RIREF"},
	{.d = OHM_ROFS, .g = +1.0/OHM_IREF, .unit = "R", .name = "RAUXL"},
	{.d = OHM_ROFS, .g = +1.0/OHM_IREF, .unit = "R", .name = "RAUXR"},
	{.d = OHM_ROFS, .g = +1.0/OHM_IREF, .unit = "R", .name = "RAUXD"},
	{.d = OHM_ROFS, .g = +1.0/OHM_IREF, .unit = "R", .name = "RAUXG"},
	{.d = OHM_ROFS, .g = +1.0/OHM_IREF, .unit = "R", .name = "ROTG"},
};

#define DMM_CH_LIST_SIZE (sizeof(dmm_ch_list) / sizeof(dmm_ch_list[0]))
#define DMM_CAL_LIST_SIZE (sizeof(dmm_cal_rom) / sizeof(dmm_cal_rom[0]))
static dmm_cal_t dmm_cal_list[DMM_CAL_LIST_SIZE] __nvm;

dmm_cal_t *dmm_cal_search(const char *ch_name) {
	dmm_cal_t *cal = NULL;
	for(int i = 0; i < DMM_CAL_LIST_SIZE; i ++) {
		if(!strcmp(ch_name, dmm_cal_list[i].name)) {
			cal = &dmm_cal_list[i];
			break;
		}
	}
	return cal;
}

const dmm_cal_t *dmm_cal_search_rom(const char *ch_name) {
	const dmm_cal_t *cal = NULL;
	for(int i = 0; i < DMM_CAL_LIST_SIZE; i ++) {
		if(!strcmp(ch_name, dmm_cal_rom[i].name)) {
			cal = &dmm_cal_rom[i];
			break;
		}
	}
	return cal;
}

const dmm_ch_t *dmm_ch_search(const char *ch_name)
{
	const dmm_ch_t *ch = NULL;
	for(int i = 0; i < DMM_CH_LIST_SIZE; i ++) {
		if(!strcmp(ch_name, dmm_ch_list[i].name)) {
			ch = &dmm_ch_list[i];
			break;
		}
	}
	return ch;
}

int dmm_switch_ch(const char *ch_name)
{
	const dmm_ch_t *ch = dmm_ch_search(ch_name);
	if(ch == NULL) {
		sys_emsg = "invalid channel name";
		return -1;
	}

	//turn off all channels
	gpio_set("DG408_S4", 0);
	gpio_set("DG408_S3", 0);
	//sys_mdelay(1);

	//cd4051@diag
	gpio_set("DIAG_S2", ch->diagx & 0x04);
	gpio_set("DIAG_S1", ch->diagx & 0x02);
	gpio_set("DIAG_S0", ch->diagx & 0x01);

	//cd4051@cdp
	gpio_set("VCDP_S2", ch->vcdpx & 0x04);
	gpio_set("VCDP_S1", ch->vcdpx & 0x02);
	gpio_set("VCDP_S0", ch->vcdpx & 0x01);

	//dg408
	gpio_set("DG408_S4", ch->dg408 & 0x20);
	gpio_set("DG408_S3", ch->dg408 & 0x10);
	gpio_set("DG408_S2", ch->dg408 & 0x04);
	gpio_set("DG408_S1", ch->dg408 & 0x02);
	gpio_set("DG408_S0", ch->dg408 & 0x01);

	//g0/d0 = default calibration parameters
	dmm_cal.g = 1;
	dmm_cal.d = 0;

#if CONFIG_DMM_INT == 1
	dmm_switch_bank(ch->dg409);
	dmm_hw_config(ch->ainx, ch->gain);
#endif

	/* merge bank & channel calibration data
	dmm_i = (dmm_v - d1) * g1 //dmm_v = g0 * vadc + d0
	      = (g0 * vadc + d0 - d1) * g1
		  = vadc * (g0 * g1) + (d0 - d1) * g1
	    g = g0 * g1
		d = (d0 - d1) * g1
	*/
	dmm_cal_t *cal = dmm_cal_search(ch_name);
	if(cal != NULL) {
		float d0 = dmm_cal.d;
		float g0 = dmm_cal.g;
		float d1 = cal->d;
		float g1 = cal->g;

		float g = g0 * g1;
		float d = g1 * (d0 - d1);
		memcpy(&dmm_cal, cal, sizeof(dmm_cal));
		dmm_cal.g = g;
		dmm_cal.d = d;
	}

	dmm_ch = ch;
	return 0;
}

//dmm hardware driver
#if CONFIG_DMM_INT == 1
static tm770x_t *dmm_hw;
static fifo_t dmm_hw_fifo;
static int dmm_hw_ainx;
static int dmm_hw_gain;

int dmm_hw_config(int ainx, int gain)
{
	dmm_hw_ainx = ainx;
	dmm_hw_gain = gain;

	DMM_ISR_N();
	tm770x_config(dmm_hw, dmm_hw_gain, TM_BUF_N, TM_BIPOLAR);
	tm770x_cal_self(dmm_hw, dmm_hw_ainx);
	fifo_dump(&dmm_hw_fifo);
	DMM_ISR_Y();
	return 0;
}

/* return nitems available inside fifo
*/
int dmm_hw_poll(void)
{
	DMM_ISR_N();
	int nitems = fifo_size(&dmm_hw_fifo);
	DMM_ISR_Y();
	return nitems;
}

/* this routine will not consider ina128 & dg408 & dg409
return nitems read, out para *result is tm7707 input voltage with unit: v
*/
int dmm_hw_read(float *v)
{
	int digv, npop = 0;
	DMM_ISR_N();
	npop = fifo_pop(&dmm_hw_fifo, &digv);
	DMM_ISR_Y();

	if(npop > 0) {
		digv = (digv - 0x800000) / dmm_hw_gain;
		*v = ADC_REF * digv / (1 << 23);
	}
	return npop;
}

int dmm_read(float *result)
{
	float v = 0;
	int n = dmm_hw_read(&v);
	if(n > 0) {
		v = v * dmm_cal.g + dmm_cal.d;
		*result = v;
	}
	return n;
}

int dmm_flush(int n)
{
	float v = 0;
	for(int i = 0; i < n;) {
		if(dmm_read(&v)) {
			i ++;
		}
	}
	return 0;
}

void EXTI4_IRQHandler(void)
{
	int data = tm770x_read(dmm_hw, dmm_hw_ainx);
	EXTI->PR = EXTI_Line4;

	//success only when fifo has space, or npush = 0
	int npush = fifo_push_force(&dmm_hw_fifo, data);
}

void dmm_hw_init(void)
{
	fifo_init(&dmm_hw_fifo, 16);
	dmm_hw = sys_malloc(sizeof(tm770x_t));
	sys_assert(dmm_hw != NULL);
	dmm_hw->bus = &spi1,
	dmm_hw->mclk_hz = DMM_MCLK_HZ;
	dmm_hw->pin_nrdy = gpio_bind(GPIO_IPU, "PC04", "TM_RDY#");
	dmm_hw->pin_nrst = gpio_bind(GPIO_PP0, "PC05", "TM_RST#");
	dmm_hw->pin_nss = gpio_bind(GPIO_PP1, "PA04", "TM_NSS#");

	//EXTI of nrdy
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	EXTI_InitTypeDef EXTI_InitStruct;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);
	EXTI_InitStruct.EXTI_Line = EXTI_Line4;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_SetPriority(SysTick_IRQn, 0);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	DMM_ISR_N();

	//init tm7707 mclk=4MHz, range: 500Khz ~ 5MHz
	const pwm_cfg_t mclk_cfg = {.hz = DMM_MCLK_HZ, .fs = 1};
	pwm33.init(&mclk_cfg);
	pwm33.set(1);

	tm770x_init(dmm_hw, DMM_ODAT_HZ);
	//tm770x_config(&dmm_hw, TM_GAIN(4), TM_BUF_N, TM_UNIPOLAR);
	//tm770x_cal_self(&dmm_hw, TM_AIN1);
	//tm770x_cal_self(&dmm_hw, TM_AIN2);
	dmm_hw_config(TM_AIN1, TM_GAIN(1));
}

int dmm_switch_bank(int bank)
{
	#if CONFIG_FDPLC_V1P0 == 1
	gpio_set("DG409_S1", bank & 0x02);
	gpio_set("DG409_S0", bank & 0x01);
	#else
	bank = BANK_ANY;
	#endif

	sys_assert(bank < NR_OF_BANK);
	dmm_cal_t *cal = &dmm_cal_list[bank];
	memcpy(&dmm_cal, cal, sizeof(dmm_cal));
	return 0;
}

int dmm_cal_bank(int bank)
{
	float x0 = 0, x1 = ADC_REF;
	float y0 = 0, y1 = 0, y;
	int N = 50;

	//apply 0v to input
	printf("self calibrating bank %s ...\n", dmm_cal_rom[bank].name);
#if CONFIG_FDPLC_V1P0 == 1
	dmm_switch_ch("VBAT-");
#else
	dmm_switch_ch("GND");
#endif
	dmm_switch_bank(bank);
	dmm_hw_config(TM_AIN1, 1);
	dmm_flush(DMM_DUMMY_N);

	//restore rom calibration data
	const dmm_cal_t *cal_rom = &dmm_cal_rom[bank];
	sys_assert(cal_rom != NULL);
	memcpy(&dmm_cal, cal_rom, sizeof(dmm_cal));

	for(int i = 0; i < N;) {
		if(dmm_read(&y)) {
			i ++;
			y0 += y;
			printf("dmm: %+.6f => %+.6f V, uncal\n", x0, y);
		}
	}

	//apply VREF_3V00 to input
	dmm_switch_ch("VREF_3V0");
	dmm_switch_bank(bank);
	dmm_hw_config(TM_AIN1, 1);
	dmm_flush(DMM_DUMMY_N);

	for(int i = 0; i < N;) {
		if(dmm_read(&y) > 0) {
			i ++;
			y1 += y;
			printf("dmm: %+.6f => %+.6f V, uncal\n", x1, y);
		}
	}

	y0 /= N;
	y1 /= N;
	printf("dmm: x0=%+.6f, y0=%+.6f, uncal\n", x0, y0);
	printf("dmm: x1=%+.6f, y1=%+.6f, uncal\n", x1, y1);

	/*linearity correction
	y0 * g + d = x0(idea value)
	y1 * g + d = x1(idea value)

	(y1 - y0) * g = (x1 - x0) => g = (x1 - x0) / (y1 - y0)
	d = x0 - y0 * g
	*/
	float g = (x1 - x0) / (y1 - y0);
	float d = x0 - y0 * g;
	printf("dmm: g=%+.6f, d=%+.6f\n", g, d);

	/*write back
	y = (x * g0 + d0) * g + d = x * (g0 * g) + (d0 * g + d)
	g1 = g0 * g
	d1 = d0 * g + d
	*/
	float g1 = dmm_cal.g * g;
	float d1 = dmm_cal.d * g + d;

	printf("dmm: g=%+.6f, d=%+.6f, storing ...\n", g1, d1);
	dmm_cal_t *cal_bank = &dmm_cal_list[bank];
	cal_bank->g = g1;
	cal_bank->d = d1;
	return 0;
}

int dmm_cal_zero(const char *ch_name)
{
	const dmm_ch_t * ch = dmm_ch_search(ch_name);
	sys_assert(ch != NULL);
	dmm_cal_t * cal = dmm_cal_search(ch_name);
	if((cal == NULL) || (cal->unit[0] != 'A')) {
		sys_ecode |= 1 << E_CAL_ZERO;
		return -1;
	}

	//apply IBAT/IQBAT/IELOAD1/.. to input
	printf("zero point calibrating %s ...\n", ch_name);
	int N = 50;
	float y0, y;

	dmm_switch_ch(ch_name);
	dmm_flush(DMM_DUMMY_N);

	for(int i = 0; i < N;) {
		if(dmm_read(&y)) {
			i ++;
			y0 += y;
			printf("%s: 0 => %+.6f %s, uncal\n", ch_name, y, cal->unit);
		}
	}

	y0 /= N;
	printf("%s: 0 => %+.6f %s, uncal\n", ch_name, y0, cal->unit);

	/* during dmm_switch_ch() formular is:
	dmm_i = (dmm_v - d1) * g1 //dmm_v = g0 * vadc + d0
	      = (g0 * vadc + d0 - d1) * g1
		  = vadc * (g0 * g1) + (d0 - d1) * g1

	so:
	   d1 = d1_original + (y0 / g1)
	*/
	float g1 = cal->g;
	cal->d = cal->d + y0 / g1;

	//after cal
	dmm_switch_ch(ch_name);
	for(int i = 0; i < N;) {
		if(dmm_read(&y)) {
			i ++;
			y0 += y;
			printf("%s: 0 => %+.6f %s, cal\n", ch_name, y, cal->unit);
		}
	}
	return 0;
}
#endif

//dmm algo, include mux selection
void dmm_init(void)
{
	#if CONFIG_DMM_INT == 1
	dmm_hw_init();
	#endif

	if(nvm_is_null()) {
		memcpy(dmm_cal_list, dmm_cal_rom, sizeof(dmm_cal_rom));
	}

	//default switch to VREF channel
	dmm_switch_ch("VREF_3V0");
}

static int cmd_dmm_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"dmm ch [ch]		get/set dmm measure channel\n"
#if CONFIG_DMM_INT == 1
		"dmm read [ch] [N]	read result voltage/current\n"
		"dmm poll		poll nr of results in fifo\n"
#endif
	};

	int ecode = 0;
	if(argc >= 2) {
		if(!strcmp(argv[1], "ch")) {
			if(argc == 2) { //list current ch_list and current ch
				if(dmm_ch != NULL) printf("<+0, %s\n", dmm_ch->name);
				else printf("<+0, NONE\n");
				return 0;
			}
			ecode = dmm_switch_ch(argv[2]);
			//dmm_flush(DMM_DUMMY_N);
		}
#if CONFIG_DMM_INT == 1
		if(!strcmp(argv[1], "poll")) {
			int nitems = dmm_poll();
			printf("<%+d, samples available in fifo\n", nitems);
			return 0;
		}
		if(!strcmp(argv[1], "read")) {
			int N = 1;
			if(argc > 3) {
				if(isdigit(argv[2][0])) {
					N = atoi(argv[2]);
					dmm_switch_ch(argv[3]);
				}
				else {
					dmm_switch_ch(argv[2]);
					N = atoi(argv[3]);
				}
			}
			else if(argc == 3){
				if(isdigit(argv[2][0])) N = atoi(argv[2]);
				else dmm_switch_ch(argv[2]);
			}
			float v = 0.0;
			for(int i = 0; i < N;) {
				sig_update();
				if(dmm_read(&v)) {
					printf("<%+.6f, unit: %s, %s\n", v, dmm_cal.unit, dmm_ch->name);
					i ++;
				}
			}
			return 0;
		}
#endif
	}

	if(argc == 1) {
		printf("%s\n", usage);
		printf("dmm_ch_list[%d] = {\n", DMM_CH_LIST_SIZE);
		for(int i = 0; i < DMM_CH_LIST_SIZE; i ++) {
			printf("    %-10s: {.dg408=0x%02x, .dg409=%d, .diagx=%d, .gain=%03d},\n",
				dmm_ch_list[i].name,
				dmm_ch_list[i].dg408,
				dmm_ch_list[i].dg409,
				dmm_ch_list[i].diagx,
				dmm_ch_list[i].gain
			);
		}
		printf("}\n");
	}
	else {
		if(ecode) printf("<%+d, %s\n", ecode, sys_emsg);
		else printf("<+0, OK\n");
	}
	return 0;
}

cmd_t cmd_dmm = {"dmm", cmd_dmm_func, "dmm i/f cmds"};
DECLARE_SHELL_CMD(cmd_dmm)

#if CONFIG_DMM_INT == 1
static int cmd_cal_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"cal self		to do self calibration\n"
	};

	int ecode = -1;
	if(argc == 2) {
		if(!strcmp(argv[1], "self")) {
			gpio_set("EVBATP", 0);
			mdelay(20);

			//restore rom cal settings
			memcpy(dmm_cal_list, dmm_cal_rom, sizeof(dmm_cal_rom));

			//bank: gain & zero point calibration
		#if CONFIG_FDPLC_V1P0 == 1
			dmm_cal_bank(BANK_18V);
			dmm_cal_bank(BANK_06V);
			dmm_cal_bank(BANK_ACS);
		#else
			dmm_cal_bank(BANK_ANY);
		#endif
			//channel: zero point self calibration
			dmm_cal_zero("IELOAD1");
			dmm_cal_zero("IELOAD2");
			dmm_cal_zero("IELOAD3");
			dmm_cal_zero("IBAT");
			dmm_cal_zero("IQBAT");
			dmm_cal_zero("I0BAT");
			nvm_save();
			ecode = 0;
		}
	}

	if(argc == 1) {
		printf("%s\n", usage);
		printf("dmm_cal_list[%d] = {\n", DMM_CAL_LIST_SIZE);
		for(int i = 0; i < DMM_CAL_LIST_SIZE; i ++) {
			printf("    %-10s: {.g = %.6f, .d = %.6f %s},\n",
				dmm_cal_list[i].name,
				dmm_cal_list[i].g,
				dmm_cal_list[i].d,
				dmm_cal_list[i].unit
			);
		}
		printf("}\n");
	}
	else {
		if(ecode) printf("<%+d, %s\n", ecode, sys_emsg);
		else printf("<+0, OK\n");
	}
	return 0;
}

cmd_t cmd_cal = {"cal", cmd_cal_func, "cal i/f cmds"};
DECLARE_SHELL_CMD(cmd_cal)
#endif
