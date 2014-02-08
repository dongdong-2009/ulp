/*
*	dmm program based on aduc processor
*	1, do adc convert continuously
*	2, serv for spi communication
*
*/
#include "config.h"
#include "ulp/sys.h"
#include "ybs_dmm.h"
#include "spi.h"
#include "common/vchip.h"
#include "common/circbuf.h"
#include "aduc706x.h"
#include <string.h>
#include "aduc_adc.h"

//#define CONFIG_SPI_DEBUG
#define CONFIG_R_REF 100000 //unit: mohm

#define vth_max	((1 << 23) * 0.7)
#define vth_min ((1 << 23) * 0.3)
#define RELAY_V_MODE() do {GP2SET = 1 << (16 + 0);} while(0)
#define RELAY_R_MODE() do {GP2CLR = 1 << (16 + 0);} while(0)

static struct dmm_reg_s dmm_ri, dmm_ro;
/*mask all the auto clear bit, special operation will be done inside vchip.c*/
static const struct dmm_reg_s dmm_az = {
	.mode.cal = 1,
	.mode.clr = 1,
	.mode.dac = 1,
};

void reg_init(void)
{
	memset(&dmm_ri, 0x00, sizeof(dmm_ri));
	memset(&dmm_ro, 0x00, sizeof(dmm_ro));
	dmm_ri.mode.bank = DMM_V_AUTO;
	vchip_init(&dmm_ri, &dmm_ro, &dmm_az, sizeof(dmm_ro));

	/*copy from mbi5025, both of them use the same spi bus except signal cs*/
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
	};
	spi0.init(&cfg);
}

int reg_update(void)
{
	int exit = 0;

#ifdef CONFIG_SPI_DEBUG
	extern circbuf_t rxfifo;
	extern circbuf_t txfifo;
	if(buf_size(&rxfifo) > 0) {
		mdelay(50);
		char byte;
		buf_pop(&rxfifo, &byte, 1);
		printf("0x%02x ", ((unsigned)byte) & 0xff);
	}
	if(buf_size(&txfifo) > 0) {
		mdelay(50);
		char byte;
		buf_pop(&txfifo, &byte, 1);
		printf("[0x%02x] ", ((unsigned)byte) & 0xff);
	}
#endif

	if(dmm_ri.mode.bank != dmm_ro.mode.bank) {
		if(dmm_ri.mode.bank != DMM_KEEP) {
			dmm_ro.mode.ready = 0;
			exit = 1;
		}
	}

	if(dmm_ri.mode.cal != dmm_ro.mode.cal) {
		dmm_ro.mode.ready = 0;
		exit = 1;
	}

	if(dmm_ri.mode.clr != dmm_ro.mode.clr) {
		dmm_ro.mode.clr = dmm_ri.mode.clr;
		dmm_ro.mode.ready = 0;
	}

	if(dmm_ri.mode.dac != dmm_ro.mode.dac) {
		DACDAT = DMM_UV_TO_D(dmm_ri.value); //default 1V
		dmm_ro.mode.dac = dmm_ri.mode.dac;
		dmm_ro.mode.ready = 0;
	}

	return exit;
}

int dmm_measure(unsigned adc, int *value)
{
	int ecode = 0;
	do {
		ecode = aduc_adc_get(adc, value);
		sys_update();
		if(reg_update()) {
			//forced quit, ecode > 0
			ecode = 1;
			break;
		}
	} while(ecode > 0);

	/*0->OK, 1->QUIT, -1->OVERRANGE*/
	return ecode;
}

void dmm_measure_v_auto(void)
{
	aduc_adc_cfg_t new, now = {.adc0 = 1, .adc1 = 0, .iexc = 0, .ua10 = 0, .pga0 = 0, .mux0 = ADUC_MUX0_DCH01};
	new.value = now.value;
	aduc_adc_init(&new, ADUC_CAL_NONE);

	while(1) {
		if(now.value != new.value) {
			aduc_adc_init(&new, ADUC_CAL_NONE);
			now.value = new.value;
		}

		int v0, ov0 = dmm_measure(ADUC_ADC0, &v0);
		if(ov0 > 0) {
			//forced quit
			break;
		}

		int d = (v0 > 0) ? v0 : -v0;
		if(d < vth_min) { //increase gain
			new.pga0 += (new.pga0 == 9) ? 0 : 1;
		}
		else if(d > vth_max) { //decrease gain
			new.pga0 -= (new.pga0 == 0) ? 0 : 1;
		}

		long long uv = ((long long) v0 * 1200 * 1000);
                uv >>= (23 + now.pga0);
		dmm_ro.value = (int) uv;
		dmm_ro.mode.ready = 1;
		dmm_ro.mode.over = (ov0) ? 1 : 0;
	}
}

void dmm_measure_r_auto(void)
{
	aduc_adc_cfg_t new, now = {.adc0 = 1, .adc1 = 1, .iexc = 5, .ua10 = 0, .pga0 = 0, .pga1 = 0, .mux0 = ADUC_MUX0_DCH01, .mux1 = ADUC_MUX1_DCH23};
	new.value = now.value;
	aduc_adc_init(&new, ADUC_CAL_NONE);

	while(1) {
		if(now.value != new.value) {
			aduc_adc_init(&new, ADUC_CAL_NONE);
			now.value = new.value;
		}

		int v0, ov0 = dmm_measure(ADUC_ADC0, &v0);
		if(ov0 > 0) {
			//forced quit
			break;
		}

		int d = (v0 > 0) ? v0 : -v0;
		if(d < vth_min) { //increase gain
			if(new.iexc == 5) {
				new.pga0 += (new.pga0 == 9) ? 0 : 1;
			}
			else {
				new.iexc ++;
			}
		}
		else if(d > vth_max) { //decrease gain
			if(new.pga0 == 0)  {
				new.iexc -= (new.iexc == 1) ? 0 : 1;
			}
			else {
				new.pga0 --;
			}
		}


		int v1, ov1 = dmm_measure(ADUC_ADC1, &v1);
		if(ov1 > 0) {
			//forced quit
			break;
		}

		d = (v1 > 0) ? v1 : -v1;
		if(d < vth_min) { //increase gain
			new.pga1 += (new.pga1 == 3) ? 0 : 1;
		}
		else if(d > vth_max) { //decrease gain
			new.pga1 -= (new.pga1 == 0) ? 0 : 1;
		}

		v1 = (v1 == 0) ? 1 : v1; /*to avoid div0 error*/
		long long mohm = (long long) v0 * CONFIG_R_REF;
		mohm <<= now.pga1;
		mohm /= v1;
		mohm >>= now.pga0;
		mohm = (mohm < 0) ? - mohm : mohm;
		dmm_ro.mode.ready = 1;
		dmm_ro.mode.over = (ov0 || ((mohm >> 32) != 0)) ? 1 : 0;
		dmm_ro.value = (dmm_ro.mode.over) ? 0x7fffffff : mohm;
	}
}

void dmm_measure_r_short(void)
{
	/*iexc = 1mA, pga0 = 512, pga1 = 8 => VR = 100mV*8 = 800mV */
	static const aduc_adc_cfg_t cfg = {.adc0 = 1, .adc1 = 1, .iexc = 5, .ua10 = 0, .pga0 = 9, .pga1 = 3, .mux0 = ADUC_MUX0_DCH01, .mux1 = ADUC_MUX1_DCH23};
	aduc_adc_init(&cfg, ADUC_CAL_NONE);

	while(1) {
		int v0, ov0 = dmm_measure(ADUC_ADC0, &v0);
		if(ov0 > 0) {
			//forced quit
			break;
		}

		int v1, ov1 = dmm_measure(ADUC_ADC1, &v1);
		if(ov1 > 0) {
			//forced quit
			break;
		}

		v1 = (v1 == 0) ? 1 : v1; /*to avoid div0 error*/
		long long mohm = (long long) v0 * CONFIG_R_REF;
		mohm <<= cfg.pga1;
		mohm /= v1;
		mohm >>= cfg.pga0;
		mohm = (mohm < 0) ? - mohm : mohm;
		dmm_ro.mode.ready = 1;
		dmm_ro.mode.over = (ov0 || ((mohm >> 32) != 0)) ? 1 : 0;
		dmm_ro.value = (dmm_ro.mode.over) ? 0x7fffffff : mohm;
	}
}

void dmm_measure_r_open(void)
{
	/*measure vx with adc0: iexc = 10uA, r=???very big, pga = min*/
	static const aduc_adc_cfg_t cfg0 = {.adc0 = 1, .adc1 = 0, .iexc = 0, .ua10 = 1, .pga0 = 0, .mux0 = ADUC_MUX0_DCH01};
	/*measure vr with adc0: iexc = 10uA, r=100ohm, pga0 = 512 => 512mV */
	static const aduc_adc_cfg_t cfg1 = {.adc0 = 1, .adc1 = 0, .iexc = 0, .ua10 = 1, .pga0 = 9, .mux0 = ADUC_MUX0_DCH23};

	while(1) {
		aduc_adc_init(&cfg0, ADUC_CAL_NONE);
		int v0, ov0 = dmm_measure(ADUC_ADC0, &v0);
		if(ov0 > 0) {
			//forced quit
			break;
		}

		aduc_adc_init(&cfg1, ADUC_CAL_NONE);
		int v1, ov1 = dmm_measure(ADUC_ADC0, &v1);
		if(ov1 > 0) {
			//forced quit
			break;
		}

		v1 = (v1 == 0) ? 1 : v1; /*to avoid div0 error*/
		long long mohm = (long long) v0 * CONFIG_R_REF;
		mohm <<= cfg1.pga0;
		mohm /= v1;
		mohm >>= cfg0.pga0;
		mohm = (mohm < 0) ? - mohm : mohm;
		dmm_ro.mode.ready = 1;
		dmm_ro.mode.over = (ov0 || ((mohm >> 32) != 0)) ? 1 : 0;
		dmm_ro.value = (dmm_ro.mode.over) ? 0x7fffffff : mohm;
	}
}

char dmm_cal(char bank, int value)
{
	return 0;
}

void dmm_init(void)
{
	//p2.0 voltage<1>/resistor<0> mode switch
	GP2DAT |= 1 << (24 + 0); //DIR = OUT
	DACCON = (1 << 4); //12bit mode, 0~1V2
	DACDAT = DMM_UV_TO_D(1000*1000); //default 1V
	aduc_adc_cfg_t cfg = {.adc0 = 1, .adc1 = 1, .pga0 = 0, .pga1 = 0};
	aduc_adc_init(&cfg, ADUC_CAL_FULL);
	#if 0
	printf("ADC0OF = %d, ADC0GN = %d, ADC1OF = %d, ADC1GN = %d\n", ADC0OF, ADC0GN, ADC1OF, ADC1GN);
	cfg.adc0 = 0;
	cfg.adc1 = 0;
	aduc_adc_init(&cfg, ADUC_CAL_NONE);
	printf("ADC0OF = %d, ADC0GN = %d, ADC1OF = %d, ADC1GN = %d\n", ADC0OF, ADC0GN, ADC1OF, ADC1GN);
	cfg.adc0 = 1;
	cfg.adc1 = 1;
	aduc_adc_init(&cfg, ADUC_CAL_NONE);
	printf("ADC0OF = %d, ADC0GN = %d, ADC1OF = %d, ADC1GN = %d\n", ADC0OF, ADC0GN, ADC1OF, ADC1GN);
	for(int i = 0; i < 10; i ++) {
		aduc_adc_init(&cfg, ADUC_CAL_FULL);
		printf("ADC0OF = %d, ADC0GN = %d, ADC1OF = %d, ADC1GN = %d\n", ADC0OF, ADC0GN, ADC1OF, ADC1GN);
	}
	#endif
}

void dmm_update(void)
{
	if(dmm_ro.mode.cal != dmm_ri.mode.cal) {
		dmm_ro.mode.ecode = dmm_cal(dmm_ro.mode.bank, dmm_ro.value);
		dmm_ro.mode.cal = dmm_ri.mode.cal;
		dmm_ro.mode.ready = 1;
	};

	if(dmm_ri.mode.bank != dmm_ro.mode.bank) {
		if(dmm_ri.mode.bank != DMM_KEEP) {
			dmm_ro.mode.bank = dmm_ri.mode.bank;
			dmm_ro.mode.ready = 0;
		}
	}

	char bank = dmm_ro.mode.bank;
	switch(bank) {
	case DMM_V_AUTO:
		RELAY_V_MODE();
		dmm_measure_v_auto();
		break;
	case DMM_R_AUTO:
		RELAY_R_MODE();
		dmm_measure_r_auto();
		break;
	case DMM_R_SHORT:
		RELAY_R_MODE();
		dmm_measure_r_short();
		break;
	case DMM_R_OPEN:
		RELAY_R_MODE();
		dmm_measure_r_open();
		break;
	default:
		return;
	}
}

int main(void)
{
	sys_init();
	reg_init();
	dmm_init();
	while(1) {
		sys_update();
		reg_update();
		dmm_update();
	}
}
