/*
*
* miaofng@2017-07-16 routine for vw
* 1, vw system includes vw and vwhost
* 2, all cylinders are controled by vwhost, vw acts like a plc io extend module
* 3, handshake between test program and vw: rdy4tst/test_end/test_ng
* 4, vw can not request test unit ID
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

typedef struct {
	const pwm_bus_t *pwm;
	const char *ELOADx_EN;
	const char *ELOADx_R1K;
	const char *ELOADx_R3A;
	int imax;
	int cal;
} eload_t;

typedef struct {
	int cal;
	char dg408;
	char dg409;
	struct {
		char ainx;
		char gain;
	} tm770x;
	const char *name;
} vw_ch_t;

#define vref_mv 3000
#define V2I(v) (v / 0.1 /*+ vw_iofs*/) //rsense = 0.1R
#define V2V(v) (v/* *vw_div_ratio*/)
//static float vw_div_ratio = (100 + 10) / 10; //100K|10K
//static float vw_iofs = 0.0;
static int vw_hw_error = 0;

#define DMM_MCLK_HZ 4000000
#define DMM_ODAT_HZ 100

#define DMM_ISR_Y() do {EXTI->PR = EXTI_Line4; NVIC_EnableIRQ(EXTI4_IRQn);} while(0)
#define DMM_ISR_N() NVIC_DisableIRQ(EXTI4_IRQn)

static tm770x_t *vw_dmm;
static fifo_t vw_dmm_fifo;
static const vw_ch_t *vw_ch_current = NULL;
static vw_ch_t vw_ch_temp;

static int vw_gpio_vrev_fast = 0;

enum {
	VW_CAL_V,
	VW_CAL_IBAT,
	VW_CAL_IQBAT,
	VW_CAL_IELOAD1,
	VW_CAL_IELOAD2,
	VW_CAL_IELOAD3,

	VW_CAL_ELOAD1,
	VW_CAL_ELOAD2,
	VW_CAL_ELOAD3,
	NR_OF_CALS,
};

typedef struct { //y = g*x + d
	float g;
	float d;
} vw_cal_t;

static vw_cal_t vw_cals[NR_OF_CALS] __nvm;

#define NR_OF_ELOADS (sizeof(vw_eloads) / sizeof(vw_eloads[0]))
static const eload_t vw_eloads[] = {
	{.cal = VW_CAL_ELOAD1, .pwm = &pwm42, .imax = 3000, .ELOADx_EN = "ELOAD1_EN", .ELOADx_R1K = "ELOAD1_R1K", .ELOADx_R3A = "ELOAD1_R3A"}, //ELOAD1
	{.cal = VW_CAL_ELOAD2, .pwm = &pwm43, .imax = 3000, .ELOADx_EN = "ELOAD2_EN", .ELOADx_R1K = "ELOAD2_R1K", .ELOADx_R3A = "ELOAD2_R3A"}, //ELOAD2
	{.cal = VW_CAL_ELOAD3, .pwm = &pwm44, .imax = 3000, .ELOADx_EN = "ELOAD3_EN", .ELOADx_R1K = "ELOAD3_R1K", .ELOADx_R3A = "ELOAD3_R3A"}, //ELOAD3
};

void __sys_init(void)
{
	//VISO
	GPIO_BIND(GPIO_PP0, PA08, VISO_EN)

	//UUT
	GPIO_BIND(GPIO_PP0, PB13, VBAT_EN)
	GPIO_BIND(GPIO_PP0, PB14, VBAT_BYPASS)
	GPIO_BIND(GPIO_PP0, PC03, VREV_EN)
	vw_gpio_vrev_fast = GPIO_BIND(GPIO_PP0, PC02, VREV_FAST)
	GPIO_BIND(GPIO_PP0, PB12, UUT_ZREN)

	//ELOAD
	GPIO_BIND(GPIO_PP0, PC08, ELOAD1_EN)
	GPIO_BIND(GPIO_PP0, PC07, ELOAD2_EN)
	GPIO_BIND(GPIO_PP0, PC06, ELOAD3_EN)
	GPIO_BIND(GPIO_PP0, PB14, ELOAD1_R1K) //HW BUG, = VBAT_BYPASS
	GPIO_BIND(GPIO_PP0, PB14, ELOAD2_R1K) //HW BUG, = VBAT_BYPASS
	GPIO_BIND(GPIO_PP0, PB14, ELOAD3_R1K) //HW BUG, = VBAT_BYPASS
	GPIO_BIND(GPIO_PP0, PC12, ELOAD1_R3A)
	GPIO_BIND(GPIO_PP0, PD02, ELOAD2_R3A)
	GPIO_BIND(GPIO_PP0, PB05, ELOAD3_R3A)

	//DMM
	GPIO_BIND(GPIO_PP0, PB01, DG408_S0)
	GPIO_BIND(GPIO_PP0, PB10, DG408_S1)
	GPIO_BIND(GPIO_PP0, PB11, DG408_S2)
	GPIO_BIND(GPIO_PP0, PA00, DG409_S0)
	GPIO_BIND(GPIO_PP0, PA01, DG409_S1)

	//LED(on board)
	GPIO_BIND(GPIO_PP0, PC00, LED_R)
	GPIO_BIND(GPIO_PP0, PC01, LED_G)
	GPIO_BIND(GPIO_AIN, PA02, VBAT_13V5)
}

void led_hwSetStatus(led_t led, led_status_t status)
{
	int yes = (status == LED_ON) ? 1 : 0;
	switch(led) {
	case LED_GREEN:
		gpio_set("LED_G", yes);
		break;
	case LED_RED:
		gpio_set("LED_R", yes);
		break;
	default:
		break;
	}
}

static void vw_iso_vset(float vout)
{
	float vfb = 2.5;
	float r1 = 15.0e3;
	float r2 = 3.0e3;
	float r3 = 17.4e3;
	float rx = 10.0e3;

	float i3 = (vout - vfb) / r1 - vfb / r2;
	float vx = vfb - r3 * i3;
	float ix = (vout - vx) / rx + i3;

	float rsen = 470;
	float vpwm = ix * rsen * 3; //3 = (200K+100K)/100K

	float vdd = 3.3;
	int regv = (int)((vpwm * 1000) / vdd);
	regv = (regv > 999) ? 999 : regv;
	regv = (regv < 0) ? 0 : regv;
	pwm41.set(regv);
	gpio_set("VISO_EN", vout > 1.0 ? 1 : 0);
}

static void vw_eload_set(int idx, int iset)
{
	sys_assert((idx >= 0) && (idx < NR_OF_ELOADS));
	const eload_t *eload = &vw_eloads[idx];

	int imax = eload->imax;
	//gpio_set(eload->ELOADx_R1K, iset == 5 ? 1 : 0);
	gpio_set(eload->ELOADx_R3A, iset >= imax ? 1 : 0);

	iset = (iset >= imax) ? 0 : iset; //R3A special process
	iset = (iset == 5) ? 0 : iset; //R1K special process

	gpio_set(eload->ELOADx_EN, iset > 0 ? 1 : 0);
	if(eload->cal >= 0) {
		const vw_cal_t *cal = &vw_cals[eload->cal];
		iset = (int) (cal->g * iset + cal->d);
	}

	//iset_mA * 0.1R * 3 * 1000 / vref_mv
	int regv = iset * 300 / vref_mv;
	vw_eloads[idx].pwm->set(999 - regv);
}

static void vw_pwm_init(void)
{
	//0-999@72KHz
	const pwm_cfg_t cfg = {.hz = 100000, .fs = 999};

	//viso, 200K,100K||100N,3V3,470
	pwm41.init(&cfg);
	pwm41.set(0);

	for(int idx = 0; idx < NR_OF_ELOADS; idx ++) {
		vw_eloads[idx].pwm->init(&cfg);
		vw_eload_set(idx, 0);
	}
}

static const vw_ch_t *vw_ch_search(const char *name)
{
	static const vw_ch_t vw_chs[] = {
		{.dg408 = 0, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 1}, .cal = VW_CAL_V, .name = "VBAT"},
		{.dg408 = 1, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VUSB1"},
		{.dg408 = 2, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VUSB2"},
		{.dg408 = 3, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VUSB3"},
		{.dg408 = 4, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VSHEILD"},
		{.dg408 = 5, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VZREN"},
		{.dg408 = 6, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VIREV"},
		{.dg408 = 7, .dg409 = 0, .tm770x = {.ainx = TM_AIN1, .gain = 4}, .cal = VW_CAL_V, .name = "VREF"},

		//0-4A *0.1R = 0.4v, gain = 4
		{.dg408 = 0, .dg409 = 0, .tm770x = {.ainx = TM_AIN2, .gain = 8}, .cal = VW_CAL_IBAT, .name = "IBAT"},
		{.dg408 = 0, .dg409 = 0, .tm770x = {.ainx = TM_AIN2, .gain = 1}, .cal = VW_CAL_IQBAT, .name = "IQBAT"}, //100R*10mA = 1V
		{.dg408 = 0, .dg409 = 1, .tm770x = {.ainx = TM_AIN2, .gain = 4}, .cal = VW_CAL_IELOAD1, .name = "IELOAD1"},
		{.dg408 = 0, .dg409 = 2, .tm770x = {.ainx = TM_AIN2, .gain = 4}, .cal = VW_CAL_IELOAD2, .name = "IELOAD2"},
		{.dg408 = 0, .dg409 = 3, .tm770x = {.ainx = TM_AIN2, .gain = 4}, .cal = VW_CAL_IELOAD3, .name = "IELOAD3"},
	};

	int nr_of_ch = sizeof(vw_chs) / sizeof(vw_chs[0]);
	const vw_ch_t *ch = NULL;
	for(int i = 0; i < nr_of_ch; i ++) {
		if(name == NULL) { //list available channels
			printf("%10s: gain = %02d\n", vw_chs[i].name, vw_chs[i].tm770x.gain);
		}
		else if(strcmp(vw_chs[i].name, name) == 0) {
			ch = &vw_chs[i];
			break;
		}
	}

	return ch;
}

static void vw_ch_select(const vw_ch_t *ch)
{
	gpio_set("DG408_S0", ch->dg408 & 0x01 ? 1 : 0);
	gpio_set("DG408_S1", ch->dg408 & 0x02 ? 1 : 0);
	gpio_set("DG408_S2", ch->dg408 & 0x04 ? 1 : 0);

	gpio_set("DG409_S0", ch->dg409 & 0x01 ? 1 : 0);
	gpio_set("DG409_S1", ch->dg409 & 0x02 ? 1 : 0);
	vw_ch_current = ch;
}

void EXTI4_IRQHandler(void)
{
	EXTI->PR = EXTI_Line4;
	int data = tm770x_read(vw_dmm, vw_ch_current->tm770x.ainx);
	fifo_push(&vw_dmm_fifo, data);
}

static int vw_dmm_read(const vw_ch_t *ch, float *result)
{
	int d, n = fifo_pop(&vw_dmm_fifo, &d);
	if(n > 0) {
		d = (d - 0x800000) / ch->tm770x.gain;
		float i, v = (vref_mv/1000.0 * d) / (1 << 23);
		if(ch->tm770x.ainx == TM_AIN1) {
			v = V2V(v);
			if(ch->cal >= 0) {
				const vw_cal_t *cal = &vw_cals[ch->cal];
				v = cal->g * v + cal->d;
			}
			*result = v;
		}
		else {
			i = V2I(v);
			if(ch->cal >= 0) {
				const vw_cal_t *cal = &vw_cals[ch->cal];
				i = cal->g * i + cal->d;
			}
			*result = i;
		}
	}
	return n;
}

static void vw_dmm_config(const vw_ch_t *ch)
{
	DMM_ISR_N();
	tm770x_config(vw_dmm, vw_ch_current->tm770x.gain, TM_BUF_N, TM_BIPOLAR);
	tm770x_cal_self(vw_dmm, vw_ch_current->tm770x.ainx);
	fifo_dump(&vw_dmm_fifo);
	DMM_ISR_Y();
}

static void vw_dmm_init(void)
{
	fifo_init(&vw_dmm_fifo, 16);
	vw_dmm = sys_malloc(sizeof(tm770x_t));
	sys_assert(vw_dmm != NULL);
	vw_dmm->bus = &spi1,
	vw_dmm->mclk_hz = DMM_MCLK_HZ;
	vw_dmm->pin_nrdy = gpio_bind(GPIO_IPU, "PC04", "TM_RDY#");
	vw_dmm->pin_nrst = gpio_bind(GPIO_PP0, "PC05", "TM_RST#");
	vw_dmm->pin_nss = gpio_bind(GPIO_PP1, "PA04", "TM_NSS#");

	//EXTI of nrdy
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

	tm770x_init(vw_dmm, DMM_ODAT_HZ);
	//tm770x_config(vw_dmm, TM_GAIN(4), TM_BUF_N, TM_UNIPOLAR);
	//tm770x_cal_self(vw_dmm, TM_AIN1);
	//tm770x_cal_self(vw_dmm, TM_AIN2);
}

void vw_dmm_cal_self_v(void)
{
	const char *ch_name = "VREF";

	//self voltage calibration - error caused by serial r-div
	vw_ch_current = vw_ch_search(ch_name);
	sys_assert(vw_ch_current != NULL);
	vw_ch_select(vw_ch_current);
	vw_dmm_config(vw_ch_current);

	vw_cal_t *cal = &vw_cals[vw_ch_current->cal];
	cal->g = 11.0; //hw default: 11.0 @ 100K,10K

	float vref, sum = 0;
	int nsamples = 64;
	for(int i = 0; i < nsamples; ) {
		int n = vw_dmm_read(vw_ch_current, &vref);
		if((n > 0) && (vref > 2.0)) {
			sum += vref;
			i += 1;
			printf("dmm: %s = %.6f V, uncal\n", ch_name, vref);
		}
	}

	vref = sum / nsamples;
	float div_ratio = vref_mv / 1000.0 / vref * cal->g;
	if((div_ratio > 12.0) || (div_ratio < 10.0)) {
		printf("dmm: error detected! DG408 died?\n");
		vw_hw_error |= 1 << vw_ch_current->cal;
		div_ratio = 11.0;
	}

	cal->g = div_ratio;
	printf("dmm: #####div_ratio##### = %.6f\n", div_ratio);

	for(int i = 0; i < nsamples; ) {
		int n = vw_dmm_read(vw_ch_current, &vref);
		if(n > 0) {
			i += 1;
			printf("dmm: %s = %.6f V\n", ch_name, vref);
		}
	}
}

void vw_dmm_cal_self_i(const char *ch_name)
{
	vw_ch_current = vw_ch_search(ch_name);
	sys_assert(vw_ch_current != NULL);
	vw_ch_select(vw_ch_current);
	vw_dmm_config(vw_ch_current);

	vw_cal_t *cal = &vw_cals[vw_ch_current->cal];
	cal->d = 0.0;

	float iget, sum = 0;
	int nsamples = 64;
	for(int i = 0; i < nsamples; ) {
		int n = vw_dmm_read(vw_ch_current, &iget);
		if(n > 0) {
			sum += iget;
			i += 1;
			printf("dmm: %s = %.6f A, uncal\n", ch_name, iget);
		}
	}

	float iofs = - sum / nsamples;
	if((iofs > 0.1) || (iofs < -0.1)) {
		iofs = 0;
		printf("dmm: error detected! DG409 died?\n");
		vw_hw_error |= 1 << vw_ch_current->cal;
	}
	cal->d = iofs;
	printf("dmm: #####iofs@%s##### = %.6f A\n", ch_name, cal->d);

	for(int i = 0; i < nsamples; ) {
		int n = vw_dmm_read(vw_ch_current, &iget);
		if(n > 0) {
			i += 1;
			printf("dmm: %s = %.6f A\n", ch_name, iget);
		}
	}
}

static void vw_cal_init(int force)
{
	if(nvm_is_null() || force) {
		for(int i = 0; i < NR_OF_CALS; i ++) {
			switch(i) {
			case VW_CAL_V:
				vw_cals[i].g = 11;
				vw_cals[i].d = 0.0;
				break;
			case VW_CAL_IQBAT:
				vw_cals[i].g = 0.001; //dueto Rsense = 100R not 0.1R
				vw_cals[i].d = 0.0;
				break;
			default:
				vw_cals[i].g = 1.0;
				vw_cals[i].d = 0.0;
				break;
			}
		}
	}
	hexdump("cal", vw_cals, sizeof(vw_cals));
}

void vw_init(void)
{
	vw_cal_init(0);
	vw_pwm_init();
	vw_dmm_init();
	vw_iso_vset(13.5);
}

void vw_update(void)
{
	int vrev_fast = gpio_get_h(vw_gpio_vrev_fast);
	if(vrev_fast) {
		int vbat_en = gpio_get("VBAT_EN");
		int vbat_bypass = gpio_get("VBAT_BYPASS");
		if(vbat_en || vbat_bypass) {
			//turn off vrev_fast auto
			gpio_set_h(vw_gpio_vrev_fast, 0);
		}
	}
}

void main()
{
	sys_init();
	shell_mute(NULL);
	printf("vwpwr v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	vw_init();
	while(1){
		sys_update();
		vw_update();
	}
}

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
		"*UUID?		query the stm32 uuid\n"
		"VBATy		uut vbat = y, then hub reset\n"
		"ELOAD 1 100	eload1=100mA, eload=1-3\n"
		"MEASURE VBAT	measure specified channel, optional N samples\n"
		"CAL hex		cal = hex2bin(hex)\n"
		"CAL SELF		do self cal\n"
	};

	if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else if(!strcmp(argv[0], "ERR?")) {
		printf("<%+d, vw_hw_error\n", vw_hw_error);
		return 0;
	}
	else if(!strcmp(argv[0], "VBATy")) {
		gpio_set("VBAT_EN", 1);
		sys_mdelay(100);
		gpio_set("HUBW_RST", 0);
		gpio_set("HUBL_RST", 0);
		sys_mdelay(10);
		gpio_set("HUBW_RST", 1);
		gpio_set("HUBL_RST", 1);
		//printf("<%+d\n", uuid);
		return 0;
	}
	else if(!strncmp(argv[0], "ELOAD", 4)) {
		int e = -1;
		int eload = 0;
		int iset = 0;
		if(argc > 2) {
			sscanf(argv[1], "%d", &eload);
			sscanf(argv[2], "%d", &iset);
		}

		for(int idx = 0; idx < NR_OF_ELOADS; idx ++) {
			if((eload == 0) || (idx + 1 == eload)) {
				vw_eload_set(idx, iset);
				e = 0;
			}
		}

		if(e) printf("<-1, OP REFUSED\n");
		else printf("<+0, OK\n");
		return 0;
	}
	else if(!strncmp(argv[0], "MEAS", 4)) {
		int e = -1;
		int nsamples = 1;
		if(argc > 2) {
			sscanf(argv[2], "%d", &nsamples);
		}

		if(argc > 1) {
			const vw_ch_t *ch = vw_ch_search(argv[1]);
			if(ch != NULL) {
				e = 0;
				vw_ch_current = ch;
				vw_ch_select(vw_ch_current);
				vw_dmm_config(vw_ch_current);

				float v_or_i = 0;
				for(int i = 0; i < nsamples; ) {
					//sys_update();
					vw_update();
					int n = vw_dmm_read(vw_ch_current, &v_or_i);
					if(n > 0) {
						//if(i > 0) {
							//ignore unstable 1st sample
							printf("<%+.6f, %s\n", v_or_i, vw_ch_current->name);
						//}
						i ++;
					}
				}
			}
		}
		else {
			printf("dmm_ch_list[] = {\n");
			vw_ch_search(NULL);
			printf("}\n");
		}

		if(e) printf("<%+d, ERROR!\n", e);
		return e;
	}
	else if(!strncmp(argv[0], "CAL", 3)) {
		//warning: hex = 8bytes * NR_OF_CALS * 2 = 112bytes!!!
		int e = -1;
		if(argc > 1) {
			if(!strcmp(argv[1], "SELF")) {
				vw_cal_init(1);

				//eload cal according to experiment
				//calibration by fluke
				vw_cals[VW_CAL_ELOAD1].g = 1.040;
				vw_cals[VW_CAL_ELOAD1].d = 0;
				vw_cals[VW_CAL_ELOAD2].g = 1.040;
				vw_cals[VW_CAL_ELOAD2].d = 0;
				vw_cals[VW_CAL_ELOAD3].g = 1.040;
				vw_cals[VW_CAL_ELOAD3].d = 0;

				vw_cals[VW_CAL_IELOAD1].g = 1;
				vw_cals[VW_CAL_IELOAD1].d = 0;
				vw_cals[VW_CAL_IELOAD2].g = 1;
				vw_cals[VW_CAL_IELOAD2].d = 0;
				vw_cals[VW_CAL_IELOAD3].g = 1;
				vw_cals[VW_CAL_IELOAD3].d = 0;

				//self calibration
				vw_dmm_cal_self_v();

				//vw_dmm_cal_self_i("IELOAD1");
				//vw_dmm_cal_self_i("IELOAD2");
				//vw_dmm_cal_self_i("IELOAD3");

				gpio_set("VBAT_EN", 1);
				mdelay(100);
				vw_dmm_cal_self_i("IBAT");
				gpio_set("VBAT_EN", 0);

				gpio_set("VBAT_BYPASS", 1);
				mdelay(100);
				vw_dmm_cal_self_i("IQBAT");
				gpio_set("VBAT_BYPASS", 0);
				printf("dmm: vw_hw_error = 0x%04x\n", vw_hw_error);
				e = vw_hw_error;
			}
			else {
				int n = strlen(argv[1]) >> 1;
				if(n == sizeof(vw_cals)) {
					hex2bin(argv[1], vw_cals);
					e = 0;
				}
			}
		}
		else {
			hexdump("CAL", vw_cals, sizeof(vw_cals));
			for(int i = 0; i < NR_OF_CALS; i ++) {
				printf("%02d: .g = %.6f, .d = %.6f\n", i, vw_cals[i].g, vw_cals[i].d);
			}
			return 0;
		}

		if(e) printf("<%+d, CAL PARA ERROR!\n", e);
		else printf("<%+d, OK!\n", e);
	}
	else if(!strcmp(argv[0], "*IDN?")) {
		printf("<+0, Ulicar Technology, VWPWR V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*UUID?")) {
		unsigned uuid = *(unsigned *)(0X1FFFF7E8);
		printf("<%+d\n", uuid);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
